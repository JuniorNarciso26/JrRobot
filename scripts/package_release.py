#!/usr/bin/env python3
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from datetime import datetime, timezone

root = Path(__file__).resolve().parents[1]
out = root / 'dist'
out.mkdir(exist_ok=True)
name = 'jrbot-face-oled-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S') + '.zip'
zip_path = out / name
include_roots = ['README.md', 'INSTALAR_JRBOT_WINDOWS.bat', 'ABRIR_PAINEL_JRBOT_WINDOWS.bat', 'INSTALAR_SCANNER_I2C_WINDOWS.bat', 'CONSOLE_SERIAL_JRBOT_WINDOWS.bat', 'GRAVAR_JRBOT_SEM_MONITOR_WINDOWS.bat', 'docs', 'firmware/face_oled', 'firmware/i2c_scanner', 'tools/jrbot_frontend', 'scripts', 'references']
skip_parts = {'.git', 'build', 'dist', '__pycache__'}
with ZipFile(zip_path, 'w', ZIP_DEFLATED) as zf:
    for rel in include_roots:
        p = root / rel
        if p.is_file():
            zf.write(p, p.relative_to(root))
            continue
        for f in p.rglob('*'):
            if f.is_file() and not any(part in skip_parts for part in f.relative_to(root).parts):
                zf.write(f, f.relative_to(root))
print(zip_path)
