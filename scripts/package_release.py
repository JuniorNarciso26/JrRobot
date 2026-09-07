#!/usr/bin/env python3
"""Package tracked sources only; never offer a switch to include credentials."""
from datetime import datetime, timezone
from pathlib import Path
import subprocess
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]

def allowed(rel):
    p=Path(rel)
    if p.is_absolute() or '..' in p.parts:
        return False
    if any(x.lower() in {'.git','dist','credencial','credentials','secrets','__pycache__','managed_components'} or x.startswith('build') for x in p.parts):
        return False
    if p.name.startswith('.env') or p.name.endswith(('.local.h','.pem','.key','.p12')):
        return False
    if rel.startswith('firmware/sdkconfig') and not p.name.startswith('sdkconfig.defaults'):
        return False
    return True

def main():
    subprocess.run(['python',str(ROOT/'tools/generate_pinmap.py'),'--check'],check=True)
    raw=subprocess.check_output(['git','-C',str(ROOT),'ls-files','-z'])
    paths=[p for p in raw.decode('utf-8').split('\0') if p and allowed(p)]
    output=ROOT/'dist';output.mkdir(exist_ok=True)
    archive=output/('JrBot-HW04-'+datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S')+'.zip')
    with ZipFile(archive,'w',ZIP_DEFLATED) as z:
        for rel in sorted(paths):
            p=ROOT/rel
            if p.is_file() and not p.is_symlink():
                z.write(p,'JrBot/'+rel)
    print(archive)
    print('Arquivo de fontes locais rastreadas, nao firmware homologado. Revise dados antes de compartilhar.')

if __name__=='__main__':
    main()
