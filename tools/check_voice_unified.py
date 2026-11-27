#!/usr/bin/env python3
"""Reject missing, oversized or incorrectly placed unified voice flash images."""
from pathlib import Path
import argparse
import hashlib
import json
from prepare_babytalk_hw04 import MODEL_SHA
EXPECTED={'factory':(0x10000,0x800000),'model':(0x810000,0x1F0000),'stt_model':(0xA00000,0x600000)}
def check(build):
    build=Path(build)
    metadata=json.loads((build/'flasher_args.json').read_text())
    files={int(offset,0):Path(path) for offset,path in metadata['flash_files'].items()}
    result={}
    for name,(offset,capacity) in EXPECTED.items():
        if offset not in files: raise RuntimeError(f'{name}: imagem ausente em {offset:#x}')
        path=files[offset]
        if not path.is_absolute(): path=build/path
        size=path.stat().st_size
        if not 0<size<=capacity: raise RuntimeError(f'{name}: {size} bytes excedem/invalidam particao de {capacity}')
        if name=='stt_model' and hashlib.sha256(path.read_bytes()).hexdigest()!=MODEL_SHA:
            raise RuntimeError('BabyTalk: hash do modelo nao confere')
        result[name]={'bytes':size,'capacity':capacity,'free':capacity-size,'offset':hex(offset)}
        print(f'[OK] {name}: {size} / {capacity} bytes; livre={capacity-size}')
    # Every flashed file must end before the next partition boundary/flash end.
    boundaries=sorted({0x8000,0x9000,0xF000,*[o for o,_ in EXPECTED.values()],0x1000000})
    for offset,path in files.items():
        if not path.is_absolute(): path=build/path
        end=offset+path.stat().st_size
        limit=next((b for b in boundaries if b>offset),0x1000000)
        if offset<0 or end>limit: raise RuntimeError(f'Imagem {path.name} invade outra particao/flash')
    (build/'voice_unified_budget.json').write_text(json.dumps(result,indent=2)+'\n')
    return result
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,required=True)
    args=parser.parse_args()
    try: check(args.build_dir)
    except (OSError,ValueError,KeyError,RuntimeError) as exc:
        parser.exit(1,'[ERRO] VOICE_UNIFIED_FLASH: '+str(exc)+'\n')
