#!/usr/bin/env python3
"""Isolated host tests. Requires GCC; does not compile ESP-IDF or test a board."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent

def main():
    gcc=shutil.which('gcc')
    if not gcc: raise SystemExit('GCC is required for host tests.')
    flags=[gcc,'-std=c11','-Wall','-Wextra','-Werror','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined']
    for path in [HERE/'stubs',ROOT/'firmware/core',ROOT/'firmware/module_face',ROOT/'firmware/module_camera',ROOT/'firmware/module_portal']:
        flags.extend(['-I',str(path)])
    groups=[
        ('boot_final',[], 'boot',['offline','network','mutex_error']),
        ('camera_final',[], 'camera',['success','repeat','psram','init_error','sensor_missing','no_frame','corrupt','null_buffer','wrong_format','zero_dimensions','deinit_error','small_buffer']),
    ]
    results=[]
    with tempfile.TemporaryDirectory() as tmp:
        for name,defs,kind,scenarios in groups:
            target=Path(tmp)/name
            source=ROOT/('firmware/main/main.c' if kind=='boot' else 'firmware/module_camera/jr_camera_diag.c')
            cmd=flags+['-D'+d for d in defs]+[str(source),str(HERE/('test_'+kind+'.c')),'-o',str(target)]
            subprocess.run(cmd,check=True,capture_output=True,text=True,timeout=30)
            for scenario in scenarios:
                proc=subprocess.run([str(target),scenario],capture_output=True,text=True,timeout=10)
                results.append({'test':name+'/'+scenario,'passed':proc.returncode==0,'output':proc.stdout+proc.stderr})
                print(('PASS ' if proc.returncode==0 else 'FAIL ')+name+'/'+scenario)
    ver=(ROOT/'firmware/version.txt').read_text().strip();assert ('"'+ver+'"') in (ROOT/'firmware/core/jr_config.h').read_text()
    report={'scope':'Host mocks + UBSan only. No ESP-IDF build or physical validation.','tests':results}
    (HERE/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if all(r['passed'] for r in results) else 1
if __name__=='__main__': raise SystemExit(main())
