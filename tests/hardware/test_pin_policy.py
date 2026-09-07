#!/usr/bin/env python3
"""Host compile checks for the fixed HW04 final map. Not an ESP-IDF or hardware test."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def main():
    cc=shutil.which(os.environ.get('CC','gcc'))
    if not cc: raise SystemExit('Set CC to a host C11 compiler (GCC).')
    subprocess.run([sys.executable,str(ROOT/'tools/generate_pinmap.py'),'--check'],check=True)
    cases=[]
    for name,defines in [('final_default',{}),('stale_old_config',{'CONFIG_JR_HEADLESS_DIAGNOSTIC':1,'CONFIG_JR_AUDIO_HW04_CONFIRMED':0})]:
        with tempfile.TemporaryDirectory(prefix='jrbot_hw04_') as tmp:
            d=Path(tmp);(d/'sdkconfig.h').write_text('\n'.join(f'#define {k} {v}' for k,v in defines.items())+'\n')
            (d/'test.c').write_text(
                '#include "jr_board.h"\n'
                '_Static_assert(JR_AUDIO_BCLK_GPIO==21, "bclk");\n'
                '_Static_assert(JR_AUDIO_LRC_GPIO==47, "ws");\n'
                '_Static_assert(JR_AUDIO_DIN_GPIO==42, "dout");\n'
                '_Static_assert(JR_MIC_SD_GPIO==41, "mic_sd");\n'
                '_Static_assert(JR_AUDIO_ENABLED==1, "audio");\n'
                '_Static_assert(JR_MIC_ENABLED==1, "mic");\n'
                '_Static_assert(JR_CAMERA_ENABLED==1, "camera");\n'
                '_Static_assert(JR_OLED_ENABLED==1, "oled");\n'
                'int main(void){return 0;}\n')
            cmd=[cc,'-std=c11','-Wall','-Wextra','-Werror','-I',str(d),'-I',str(ROOT/'firmware/core'),str(d/'test.c'),'-o',str(d/'out')]
            run=subprocess.run(cmd,capture_output=True,text=True,timeout=10);ok=run.returncode==0
            cases.append({'case':name,'passed':ok});print(('PASS ' if ok else 'FAIL ')+name)
            if not ok: print(run.stderr)
    result={'scope':'Host C11 final pin-map checks only. No ESP-IDF build or physical test.','passed':sum(x['passed'] for x in cases),'failed':sum(not x['passed'] for x in cases),'cases':cases}
    (ROOT/'tests/hardware/results.json').write_text(json.dumps(result,indent=2)+'\n')
    return 1 if result['failed'] else 0
if __name__=='__main__': raise SystemExit(main())
