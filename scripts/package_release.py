#!/usr/bin/env python3
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from datetime import datetime, timezone
import argparse

parser = argparse.ArgumentParser(description='Gerar ZIP do JrBot')
parser.add_argument('--include-credencial', action='store_true', help='inclui a pasta credencial no ZIP desta geracao')
args = parser.parse_args()

root = Path(__file__).resolve().parents[1]
out = root / 'dist'
out.mkdir(exist_ok=True)
name = 'jrbot-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S') + '.zip'
zip_path = out / name
include_roots = ['README.md', 'INSTALAR.bat', 'PAINEL.bat', 'CONFIGURAR_WIFI.bat', 'DIAGNOSTICO.bat', 'docs', 'firmware/esp32', 'tools/jrbot_frontend', 'scripts', 'references']
if args.include_credencial:
    include_roots.append('credencial')
skip_parts = {'.git', 'build', 'dist', '__pycache__'}
skip_names = {'wifi_config.local.h'}
with ZipFile(zip_path, 'w', ZIP_DEFLATED) as zf:
    for rel in include_roots:
        p = root / rel
        if not p.exists():
            continue
        if p.is_file():
            zf.write(p, p.relative_to(root))
            continue
        for f in p.rglob('*'):
            rel_parts = f.relative_to(root).parts
            if f.is_file() and f.name not in skip_names and not any(part in skip_parts for part in rel_parts):
                zf.write(f, f.relative_to(root))
print(zip_path)
