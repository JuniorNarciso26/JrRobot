#!/usr/bin/env python3
"""Provision the JrBot's PRIVATE local HTTPS material for the develop firmware.

The header is gitignored. Reuse a prior local JrBot header when possible,
otherwise generate a self-signed jrbot.local certificate on this computer.
Never upload the header, key or certificate to GitHub.
"""
from __future__ import annotations

from datetime import datetime, timedelta, timezone
import os
from pathlib import Path
import shutil
import stat
import tempfile

ROOT = Path(__file__).resolve().parents[1]
RELATIVE_HEADER = Path("firmware/main/jr_https_material_local.h")
RELATIVE_PORTAL = Path("firmware/module_portal/jr_portal.c")


def _valid_header(path: Path) -> bool:
    if not path.is_file() or path.is_symlink() or path.stat().st_size > 100_000:
        return False
    try:
        text = path.read_text(encoding="utf-8")
    except (UnicodeError, OSError):
        return False
    return all(marker in text for marker in (
        "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1",
        "JR_HTTPS_CERT_PEM",
        "JR_HTTPS_KEY_PEM",
    ))


def _candidate_headers(root: Path):
    """Search only neighboring local clones, never GitHub or the network."""
    dirs = [
        root.parent,
        Path.home() / "Desktop",
        Path.home() / "OneDrive" / "Desktop",
    ]
    visited: set[Path] = set()
    for parent in dirs:
        if not parent.is_dir():
            continue
        for candidate in sorted(parent.glob("*/firmware/main/jr_https_material_local.h")):
            resolved = candidate.resolve()
            if resolved in visited or resolved == (root / RELATIVE_HEADER).resolve():
                continue
            visited.add(resolved)
            yield candidate


def _c_pem(name: str, pem: bytes) -> str:
    lines = pem.decode("ascii").splitlines()
    return "static const char " + name + "[] =\n" + "".join(
        '"' + line + r"\n" + '"' + "\n" for line in lines
    ) + ";\n"


def _generate() -> str:
    # cryptography is installed in the ESP-IDF Python environment on supported
    # ESP-IDF installations; do not modify the system Python or global trust.
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID

    key = ec.generate_private_key(ec.SECP256R1())
    name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "jrbot.local")])
    now = datetime.now(timezone.utc)
    cert = (
        x509.CertificateBuilder()
        .subject_name(name)
        .issuer_name(name)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - timedelta(days=1))
        .not_valid_after(now + timedelta(days=730))
        .add_extension(x509.SubjectAlternativeName([x509.DNSName("jrbot.local")]), critical=False)
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False)
        .sign(key, hashes.SHA256())
    )
    cert_pem = cert.public_bytes(serialization.Encoding.PEM)
    key_pem = key.private_bytes(
        serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8,
        serialization.NoEncryption(),
    )
    return (
        "/* PRIVATE JrBot HTTPS material. Generated locally; DO NOT COMMIT. */\n"
        "#ifndef JR_HTTPS_MATERIAL_LOCAL_H\n"
        "#define JR_HTTPS_MATERIAL_LOCAL_H\n"
        "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1\n"
        + _c_pem("JR_HTTPS_CERT_PEM", cert_pem)
        + _c_pem("JR_HTTPS_KEY_PEM", key_pem)
        + "#endif\n"
    )


def _atomic_private_write(target: Path, content: str) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    tmp_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="ascii", newline="\n",
            prefix=".jr_https_", suffix=".tmp", dir=target.parent,
            delete=False,
        ) as output:
            tmp_path = Path(output.name)
            output.write(content)
        os.chmod(tmp_path, stat.S_IRUSR | stat.S_IWUSR)
        os.replace(tmp_path, target)
    finally:
        if tmp_path and tmp_path.exists():
            tmp_path.unlink()


def _force_portal_rebuild(root: Path) -> None:
    # Adding a previously absent #include file does not always invalidate an
    # existing Ninja object compiled while __has_include() returned false.
    # Updating mtime only (not contents) makes Ninja recompile this source.
    portal = root / RELATIVE_PORTAL
    if portal.exists():
        os.utime(portal, None)


def prepare(root: Path = ROOT) -> int:
    target = root / RELATIVE_HEADER
    if target.exists():
        if not _valid_header(target):
            print("[ERRO] HTTPS local existente nao tem formato valido. Arquivo preservado:", target)
            return 2
        print("[OK] HTTPS local ja configurado; certificado/chave preservados.")
        return 0

    for previous in _candidate_headers(root):
        if _valid_header(previous):
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(previous, target)
            _force_portal_rebuild(root)
            print("[OK] HTTPS local recuperado da instalacao anterior:", previous.parent.parent.parent)
            print("[INFO] Certificado e chave continuam apenas neste computador.")
            return 0

    try:
        contents = _generate()
    except ImportError as error:
        print("[ERRO] Biblioteca cryptography ausente no Python do ESP-IDF:", error)
        print("[INFO] Instale/repere o ambiente Python do ESP-IDF 6.1.1 antes de gravar.")
        return 2
    except Exception as error:
        print("[ERRO] Nao foi possivel criar o certificado HTTPS local:", error)
        return 2
    _atomic_private_write(target, contents)
    _force_portal_rebuild(root)
    print("[OK] HTTPS local criado automaticamente para jrbot.local; chave privada somente no PC.")
    print("[AVISO] Certificado autoassinado: o navegador pode exigir confirmacao manual.")
    print("[AVISO] Nao exponha o painel HTTPS do JrBot a internet publica.")
    return 0


if __name__ == "__main__":
    raise SystemExit(prepare())
