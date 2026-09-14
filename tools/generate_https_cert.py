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
CA_CERT = CERT_DIR / "jrbot-dev-ca.pem"
CA_KEY = CERT_DIR / "jrbot-dev-ca-key.pem"
SERVER_CERT = CERT_DIR / "jrbot-cert.pem"
SERVER_KEY = CERT_DIR / "jrbot-key.pem"
SERVER_CSR = CERT_DIR / "jrbot.csr"
SERVER_CFG = CERT_DIR / "openssl-jrbot.cnf"


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


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def c_lines(value: str) -> str:
    parts: list[str] = []
    for line in value.splitlines(keepends=True):
        escaped = line.replace("\\", "\\\\").replace('"', '\\"').replace("\r", "").replace("\n", "\\n")
        parts.append(f'"{escaped}"')
    if not parts or not value.endswith("\n"):
        parts.append('""')
    return "\n".join(parts)


def ensure_ca(openssl: str) -> bool:
    if CA_CERT.is_file() and CA_KEY.is_file():
        return False
    for path in (CA_CERT, CA_KEY):
        if path.exists():
            path.unlink()
    run([
        openssl, "req", "-x509", "-nodes", "-newkey", "rsa:2048", "-sha256",
        "-days", "3650", "-keyout", str(CA_KEY), "-out", str(CA_CERT),
        "-subj", "/CN=JrBot Dev CA/O=JrBot HTTPS Local Experiment",
    ])
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Gera CA local e certificado HTTPS do experimento JrBot")
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
    ca_created = ensure_ca(openssl)

    SERVER_CFG.write_text(
        "[req]\n"
        "distinguished_name=dn\n"
        "prompt=no\n"
        "req_extensions=v3_req\n"
        "[dn]\n"
        f"CN={address}\n"
        "O=JrBot HTTPS Local Experiment\n"
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

    run([
        openssl, "req", "-new", "-nodes", "-newkey", "rsa:2048", "-sha256",
        "-keyout", str(SERVER_KEY), "-out", str(SERVER_CSR), "-config", str(SERVER_CFG),
    ])
    serial = CERT_DIR / "jrbot-dev-ca.srl"
    if serial.exists():
        serial.unlink()
    run([
        openssl, "x509", "-req", "-in", str(SERVER_CSR),
        "-CA", str(CA_CERT), "-CAkey", str(CA_KEY), "-CAcreateserial",
        "-out", str(SERVER_CERT), "-days", "365", "-sha256",
        "-extfile", str(SERVER_CFG), "-extensions", "v3_req",
    ])

    cert_text = SERVER_CERT.read_text(encoding="ascii")
    key_text = SERVER_KEY.read_text(encoding="ascii")
    HEADER.write_text(
        "#pragma once\n"
        "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1\n"
        "static const char JR_HTTPS_CERT_PEM[] =\n" + c_lines(cert_text) + ";\n"
        "static const char JR_HTTPS_KEY_PEM[] =\n" + c_lines(key_text) + ";\n",
        encoding="utf-8",
    )

    ca_fp = subprocess.run(
        [openssl, "x509", "-in", str(CA_CERT), "-noout", "-fingerprint", "-sha256"],
        check=True, capture_output=True, text=True,
    ).stdout.strip()
    server_fp = subprocess.run(
        [openssl, "x509", "-in", str(SERVER_CERT), "-noout", "-fingerprint", "-sha256"],
        check=True, capture_output=True, text=True,
    ).stdout.strip()

    print(f"[OK] CA local: {'criada' if ca_created else 'reutilizada'}")
    print(f"[OK] CA para instalar/confiar: {CA_CERT}")
    print(f"[OK] CA {ca_fp}")
    print(f"[OK] Certificado do JrBot gerado para {address}")
    print(f"[OK] SAN: IP:{address}, DNS:jrbot.local")
    print(f"[OK] Certificado do servidor valido por 365 dias")
    print(f"[OK] Servidor {server_fp}")
    print(f"[OK] Material HTTPS embutivel gerado em {HEADER}")
    print("[INFO] A chave privada da CA e a chave do servidor permanecem fora do Git.")
    print("[IMPORTANTE] Para Secure Context real, o dispositivo cliente deve confiar em jrbot-dev-ca.pem.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
