"""Offline browser checks; HTTP and device responses are simulated, no ESP32."""
import json
import os
from pathlib import Path
import shutil
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
RESULTS=[]
OUT=ROOT/'tests/panel/evidence';OUT.mkdir(exist_ok=True)
with sync_playwright() as p:
    browser=p.chromium.launch(executable_path=os.environ.get('CHROMIUM_PATH') or shutil.which('chromium'),headless=True,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1280,'height':900})
    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    html=(ROOT/'tools/jrbot_frontend/index.html').read_text().replace('<script src="/panel.js"></script>','').replace('{APP_VERSION}','JRBOT-PANEL-V2-02')
    page.set_content(html)
    page.evaluate('''() => {
      const store=new Map();
      Object.defineProperty(window,'localStorage',{value:{getItem:k=>store.get(k)||null,setItem:(k,v)=>store.set(k,String(v))}});
      window.mock={connected:false,fail:false,old:false,audio:'on_demand',camera:'on_demand',calls:[]};
      window.fetch=async(path,opts={})=>{
        const m=window.mock;let ok=true,text='OK';
        if(path==='/ports')text=JSON.stringify({ports:['COM6','COM4']});
        else if(path.startsWith('/logs'))text=JSON.stringify({logs:[],serial_connected:m.connected,serial_port:m.connected?'COM4':'',session:'UI_TEST',next_cursor:0});
        else if(path==='/connect'){m.connected=true;text='Serial conectado COM4';}
        else if(path==='/disconnect'){m.connected=false;text='Serial desconectado';}
        else if(path==='/send'){
          const c=opts.body.get('command');m.calls.push({command:c,mode:opts.body.get('mode')});
          if(m.fail){ok=false;text='Falha simulada; execucao nao confirmada';}
          else if(c==='status')text=m.old?'JR_STATUS v=4':'JR_STATUS protocol=2 version=JRBOT-V2-DIAG-02 profile=headless_diagnostic oled=disabled audio='+m.audio+' camera='+m.camera+' mic=not_configured volume=10';
          else if(c.startsWith('audio_volume '))text='JR_OK audio_volume='+c.split(' ')[1]+' audio=enabled';
          else if(c==='audio_test')text='JR_OK audio_test=tx_completed audible_check=pending';
          else if(c==='camera_test')text='JR_OK camera_test=frame_received pid=0x5640 width=320 height=240 bytes=4500 released=1 optical_check=pending';
          else if(c.startsWith('wifi_config_pct '))text='JR_WIFI_SALVO pending_restart=1';
          else text='JR_OK command=accepted';
        }
        return {ok,status:ok?200:502,text:async()=>text};
      };
    }''')
    page.add_script_tag(content=(ROOT/'tools/jrbot_frontend/panel.js').read_text())
    page.wait_for_function("document.querySelector('#port').value==='COM4'")
    assert page.locator('#faces button').count()==16
    assert page.locator('#test_audio').is_disabled() and page.locator('#test_camera').is_disabled()
    RESULTS.append('defaults_COM4_and_tests_disabled_until_confirmed')
    page.get_by_role('button',name='Conectar Serial',exact=True).click()
    page.wait_for_function("document.querySelector('#fw_version').textContent==='JRBOT-V2-DIAG-02'")
    assert 'V2 confirmada' in page.locator('#conn').inner_text()
    assert 'bloqueia' in page.locator('#oled_state').inner_text()
    assert page.locator('#test_audio').is_enabled() and page.locator('#test_camera').is_enabled()
    assert page.locator('#faces button[disabled]').count()==16
    RESULTS.append('connect_queries_status_and_headless_does_not_block_other_tests')
    page.locator('#test_audio').click();page.wait_for_function("!busy")
    assert 'Envio do tom concluido' in page.locator('#audio_msg').inner_text()
    assert page.evaluate("mock.calls.slice(-2).map(x=>x.command)")==['audio_volume 10','audio_test']
    RESULTS.append('sound_button_sets_low_volume_and_waits_for_ack')
    page.locator('#test_camera').click();page.wait_for_function("!busy")
    assert '320 x 240' in page.locator('#cam_msg').inner_text()
    assert page.evaluate("mock.calls.at(-1).mode")=='serial'
    RESULTS.append('camera_button_uses_Serial_and_displays_metadata')
    page.evaluate('mock.fail=true')
    page.locator('#test_audio').click();page.wait_for_function("!busy")
    assert page.locator('#audio_msg').inner_text().startswith('Erro:')
    assert 'concluido' not in page.locator('#audio_msg').inner_text()
    RESULTS.append('failed_audio_request_never_shows_success')
    page.evaluate('mock.fail=false;mock.audio="disabled";mock.camera="disabled"')
    page.evaluate('refreshStatus()')
    assert page.locator('#test_audio').is_disabled() and page.locator('#test_camera').is_disabled()
    assert 'firmware' in page.locator('#cam_msg').inner_text()
    RESULTS.append('firmware_hardware_gates_preserved')
    assert page.locator('#test_mic').is_disabled()
    RESULTS.append('microphone_not_falsely_reported_testable')
    page.evaluate('mock.audio="on_demand";mock.camera="on_demand";refreshStatus()')
    page.evaluate('mock.old=true;refreshStatus()')
    assert 'Erro:' in page.locator('#device_msg').inner_text()
    assert page.locator('#test_audio').is_disabled()
    RESULTS.append('old_firmware_not_accepted_as_V2')
    page.evaluate('mock.old=false;mock.audio="on_demand";mock.camera="on_demand";mock.connected=true')
    page.evaluate('refreshStatus()')
    page.locator('input[value="wifi"]').check()
    page.evaluate('refreshStatus()')
    assert page.locator('#test_camera').is_disabled()
    assert 'Serial USB' in page.locator('#cam_msg').inner_text()
    RESULTS.append('camera_not_sent_over_unsupported_HTTP_transport')
    before=page.evaluate('serverCursor');page.evaluate('localLine("TEST_LOCAL");clearLog()')
    assert page.evaluate('serverCursor')==before
    RESULTS.append('local_log_does_not_advance_server_cursor')
    page.evaluate('setMode("serial")');page.evaluate('mock.connected=true;refreshStatus()')
    page.locator('#wifi_ssid').fill('test-network')
    page.locator('#wifi_pass').fill('secret_for_browser_test')
    page.evaluate('configureWifi()')
    assert 'secret_for_browser_test' not in page.locator('#log').inner_text()
    assert page.locator('#wifi_pass').input_value()==''
    RESULTS.append('credentials_not_echoed_and_password_cleared_after_ack')
    page.screenshot(path=str(OUT/'desktop.png'),full_page=True)
    page.set_viewport_size({'width':390,'height':844})
    assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
    page.screenshot(path=str(OUT/'mobile.png'),full_page=True)
    RESULTS.append('mobile_no_horizontal_overflow')
    assert not errors,errors
    RESULTS.append('no_unhandled_browser_errors')
    browser.close()
(OUT/'browser_results.json').write_text(json.dumps({'scope':'Offline Chromium; simulated HTTP/device responses; no hardware','passed':len(RESULTS),'tests':RESULTS},indent=2))
print(json.dumps({'browser_tests_passed':len(RESULTS),'tests':RESULTS},indent=2))
