#!/usr/bin/env python3
"""Host compilation checks, not an ESP-IDF build or hardware test. Requires GCC."""
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
    if not cc:
        raise SystemExit('Set CC to a host C11 compiler (GCC).')
    results=[]
    subprocess.run([sys.executable,str(ROOT/'tools/generate_pinmap.py'),'--check'],check=True)
    def check(name,defines,should_compile,enabled=0):
        with tempfile.TemporaryDirectory(prefix='jrbot_hw03_') as tmp:
            d=Path(tmp)
            (d/'sdkconfig.h').write_text('\n'.join(f'#define {k} {v}' for k,v in defines.items())+'\n')
            (d/'test.c').write_text('#include "jr_board.h"\n'
                +f'_Static_assert(JR_AUDIO_ENABLED=={enabled}, "Unexpected audio approval");\n'
                +'int main(void){return 0;}\n')
            cmd=[cc,'-std=c11','-Wall','-Wextra','-Werror','-I',str(d),'-I',str(ROOT/'firmware/core'),str(d/'test.c'),'-o',str(d/'out')]
            run=subprocess.run(cmd,capture_output=True,text=True,timeout=10)
            passed=(run.returncode==0)==should_compile
            results.append({'case':name,'passed':passed,'expected_compile':should_compile})
            print(('PASS ' if passed else 'FAIL ')+name)
            if not passed: print(run.stderr)
    def cfg(a=39,b=40,c=14,confirm=1):
        return {'CONFIG_JR_HEADLESS_DIAGNOSTIC':1,'CONFIG_JR_AUDIO_BCLK_GPIO':a,
                'CONFIG_JR_AUDIO_WS_GPIO':b,'CONFIG_JR_AUDIO_DOUT_GPIO':c,
                'CONFIG_JR_AUDIO_HW03_CONFIRMED':confirm}
    check('default_unassigned',{},True)
    check('legacy_approval_ignored',{'CONFIG_JR_AUDIO_PINS_CONFIRMED':1},True)
    check('configured_not_approved',cfg(confirm=0),True)
    check('verified_candidate',cfg(),True,1)
    check('approved_but_unassigned',{'CONFIG_JR_AUDIO_HW03_CONFIRMED':1},False)
    for blocked in (21,41,42,47):
        for signal in range(3):
            for confirm in (0,1):
                pins=[39,40,14];pins[signal]=blocked
                check(f'blocked_{blocked}_signal{signal}_approval{confirm}',cfg(*pins,confirm=confirm),False)
    for reserved in (0,1,2,3,4,15,19,20,22,25,26,33,37,43,44,45,46):
        check(f'reserved_{reserved}',cfg(c=reserved),False)
    for n,pins in enumerate(((39,39,14),(39,40,39),(39,40,40))):
        check(f'duplicate_{n}',cfg(*pins),False)
    for value in (-2,49):
        check(f'out_of_range_{value}',cfg(c=value),False)
    check('candidate_38_only_after_approval',cfg(c=38),True,1)
    check('candidate_48_only_after_approval',cfg(c=48),True,1)
    result={'scope':'Host C11 compile-time pin policy only. No ESP-IDF build or physical test.',
            'passed':sum(x['passed'] for x in results),'failed':sum(not x['passed'] for x in results),'cases':results}
    (ROOT/'tests/hardware/results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('passed','failed')}))
    return 1 if result['failed'] else 0
if __name__=='__main__':
    raise SystemExit(main())
