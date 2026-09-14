#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ipaddress
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CERT_DIR = ROOT / "firmware" / "certs" / "local"
HEADER = ROOT / "firmware" / "main" / "jr_https_material_local.h"


def find_openssl() -> str:
    candidates = [
        shutil.which("openssl"),
        str(Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "Git" / "usr" / "bin" / "openssl.exe"),
        str(Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "Git" / "mingw64" / "bin" / "openssl.exe"),
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    raise SystemExit("OpenSSL nao encontrado. Instale Git for Windows ou coloque openssl no PATH.")


def c_lines(value: str) -> str:
    parts = []
    for line in value.splitlines(keepends=True):
        escaped = line.replace("\\", "\\\\").replace('"', '\\"').replace("\r", "").replace("\n", "\\n")
        parts.append(f'"{escaped}"')
    if not parts or not value.endswith("\n"):
        parts.append('""')
    return "\n".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Gera certificado HTTPS local para o experimento JrBot")
    parser.add_argument("--ip", required=True, help="IPv4 atual do JrBot na rede local")
    args = parser.parse_args()
    try:
        address = ipaddress.IPv4Address(args.ip.strip())
    except ipaddress.AddressValueError as exc:
        raise SystemExit(f"IPv4 invalido: {args.ip}") from exc
    if not address.is_private:
        raise SystemExit("Use o IPv4 privado/local atual do JrBot.")

    openssl = find_openssl()
    CERT_DIR.mkdir(parents=True, exist_ok=True)
    cert = CERT_DIR / "jrbot-cert.pem"
    key = CERT_DIR / "jrbot-key.pem"
    cfg = CERT_DIR / "openssl-jrbot.cnf"
    cfg.write_text(
        "[req]\n"
        "distinguished_name=dn\n"
        "x509_extensions=v3_req\n"
        "prompt=no\n"
        "[dn]\n"
        f"CN={address}\n"
        "[v3_req]\n"
        "basicConstraints=critical,CA:FALSE\n"
        "keyUsage=critical,digitalSignature,keyEncipherment\n"
        "extendedKeyUsage=serverAuth\n"
        "subjectAltName=@alt_names\n"
        "[alt_names]\n"
        f"IP.1={address}\n"
        "DNS.1=jrbot.local\n",
        encoding="utf-8",
    )
    cmd = [
        openssl, "req", "-x509", "-nodes", "-newkey", "rsa:2048", "-sha256",
        "-days", "30", "-keyout", str(key), "-out", str(cert), "-config", str(cfg),
    ]
    subprocess.run(cmd, check=True)
    cert_text = cert.read_text(encoding="ascii")
    key_text = key.read_text(encoding="ascii")
    HEADER.write_text(
        "#pragma once\n"
        "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1\n"
        "static const char JR_HTTPS_CERT_PEM[] =\n" + c_lines(cert_text) + ";\n"
        "static const char JR_HTTPS_KEY_PEM[] =\n" + c_lines(key_text) + ";\n",
        encoding="utf-8",
    )
    result = subprocess.run([openssl, "x509", "-in", str(cert), "-noout", "-fingerprint", "-sha256"], check=True, capture_output=True, text=True)
    print(f"[OK] Certificado HTTPS gerado para {address}")
    print(f"[OK] SAN: IP:{address}, DNS:jrbot.local")
    print(f"[OK] Validade de teste: 30 dias")
    print(f"[OK] {result.stdout.strip()}")
    print(f"[OK] Material local gerado em {HEADER}")
    print("[INFO] Certificado e chave permanecem fora do Git.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
