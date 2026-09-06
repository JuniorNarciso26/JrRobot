import importlib.util
import io
import json
import pathlib
import threading
import time
import unittest
import urllib.error
import urllib.parse
import urllib.request
from unittest.mock import patch
ROOT=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('panel',ROOT/'tools/jrbot_frontend/app.py')
app=importlib.util.module_from_spec(spec);spec.loader.exec_module(app)

class SerialMock:
    is_open=True
    def __init__(self,kind='ok'):self.kind=kind;self.written=b''
    def write(self,b):
        self.written=b
        if self.kind=='partial':return 1
        if self.kind=='error':raise OSError('broken')
        ident=b.decode().split(' ',1)[0][1:]
        if self.kind=='ok':app.receive_line('JR_REPLY id='+ident+' ok=1 JR_OK audio_volume=70')
        if self.kind=='no':app.receive_line('JR_REPLY id='+ident+' ok=0 JR_ERROR test_failure')
        if self.kind=='wrong':app.receive_line('JR_REPLY id=wrongid ok=1 JR_OK')
        return len(b)
    def close(self):self.is_open=False

class PanelTests(unittest.TestCase):
    def setUp(self):
        with app.LOG_LOCK:app.LOGS.clear();app.LOG_ID=0
        app.SERIAL=None
        with app.PENDING_LOCK:app.PENDING.clear()
    def test_secret_redaction_spaces_and_encoded(self):
        for secret in ['wifi_config ssid=Test|pass=secret with spaces|host=jrbot','PASS=secret with spaces','wifi_config_pct ssid=T|pass=secret%20x']:
            app.add_log(secret)
        self.assertNotIn('secret',json.dumps(list(app.LOGS)))
    def test_status_not_redacted(self):
        line='JR_STATUS wifi_config=1 wifi=1 oled=offline'
        app.add_log(line);self.assertEqual(app.LOGS[-1]['line'],line)
    def test_split_utf8(self):
        lines=app.SerialLines();payload='JR_WIFI conectado a\u00e7\u00e3o\n'.encode()
        for b in payload:lines.feed(bytes([b]))
        self.assertEqual(app.LOGS[-1]['line'],'JR_WIFI conectado a\u00e7\u00e3o')
    def test_overflow_discards_suffix(self):
        lines=app.SerialLines();lines.feed(b'x'*4200+b'suffix\nvalid\n')
        self.assertEqual(len(app.LOGS),2);self.assertEqual(app.LOGS[-1]['line'],'valid')
        self.assertNotIn('suffix',json.dumps(list(app.LOGS)))
    def test_ack_success(self):
        app.SERIAL=SerialMock();self.assertEqual(app.serial_request('audio_volume 70',.01),'JR_OK audio_volume=70')
        self.assertRegex(app.SERIAL.written.decode(),r'^@[a-f0-9]{16} audio_volume 70\n$');self.assertFalse(app.PENDING)
    def test_ack_error(self):
        app.SERIAL=SerialMock('no')
        with self.assertRaisesRegex(RuntimeError,'test_failure'):app.serial_request('audio_volume 70',.01)
    def test_late_or_wrong_id_does_not_confirm(self):
        app.SERIAL=SerialMock('wrong')
        with self.assertRaisesRegex(RuntimeError,'Sem confirmacao'):app.serial_request('status',.01)
    def test_no_ack_no_false_success(self):
        app.SERIAL=SerialMock('timeout')
        with self.assertRaisesRegex(RuntimeError,'incerta'):app.serial_request('status',.01)
    def test_partial_serial_write(self):
        app.SERIAL=SerialMock('partial')
        with self.assertRaisesRegex(RuntimeError,'nao confirmada'):app.serial_request('status',.01)
    def test_not_connected(self):
        with self.assertRaisesRegex(RuntimeError,'nao conectada'):app.serial_request('status',.01)
    def test_utf8_byte_limit(self):
        with self.assertRaises(ValueError):app.validate_command('\u00e9'*385)
        self.assertEqual(app.validate_command('\u00e9'*384),'\u00e9'*384)
    def test_control_rejected(self):
        for c in ['status\naudio_test','status\x00','status\r']:
            with self.assertRaises(ValueError):app.validate_command(c)
    def test_logs_pagination(self):
        for i in range(700):app.add_log('line '+str(i))
        all_ids=[];cursor=0
        for _ in range(3):
            page=app.log_page(cursor,app.SESSION);all_ids.extend(r['id'] for r in page['logs']);cursor=page['next_cursor']
        self.assertEqual(all_ids,list(range(1,701)))
    def test_logs_session_reset(self):
        app.add_log('new');p=app.log_page(999,'oldsession');self.assertEqual(p['next_cursor'],1);self.assertEqual(len(p['logs']),1)
    def test_log_concurrency(self):
        def write():
            for _ in range(100):app.add_log('event')
        threads=[threading.Thread(target=write) for _ in range(6)]
        for t in threads:t.start()
        for t in threads:t.join()
        ids=[r['id'] for r in app.LOGS];self.assertEqual(ids,list(range(1,601)))
    def test_ip_constraints(self):
        self.assertEqual(app.safe_ip('192.168.0.50'),'192.168.0.50')
        for value in ['http://192.168.0.50','127.0.0.1','8.8.8.8','192.168.0.50:8000','192.168.0.50/evil','0.0.0.0']:
            with self.assertRaises(ValueError):app.safe_ip(value)
    def test_wifi_uses_post_header(self):
        class Response:
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self,n):return b'JR_OK audio_volume=70'
        class Opener:
            def open(self,request,timeout):
                self.request=request;return Response()
        opener=Opener()
        with patch.object(app.urllib.request,'build_opener',return_value=opener):
            self.assertEqual(app.wifi_request('192.168.0.50','audio_volume 70'),'JR_OK audio_volume=70')
        self.assertEqual(opener.request.method,'POST');self.assertEqual(opener.request.data,b'audio_volume 70')
        self.assertEqual(opener.request.get_header('X-jrbot-command'),'1')
    def test_wifi_rejects_foreign_success_page(self):
        class Response:
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self,n):return b'<html>Not a JrBot</html>'
        with patch.object(app.urllib.request,'build_opener') as factory:
            factory.return_value.open.return_value=Response()
            with self.assertRaisesRegex(RuntimeError,'protocolo'):app.wifi_request('192.168.0.50','status')
    def test_wifi_rejects_unchanged_volume(self):
        class Response:
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self,n):return b'JR_OK audio_volume=35'
        with patch.object(app.urllib.request,'build_opener') as factory:
            factory.return_value.open.return_value=Response()
            with self.assertRaisesRegex(RuntimeError,'diverge'):app.wifi_request('192.168.0.50','audio_volume 70')
    def test_wifi_rejects_provisioning(self):
        with self.assertRaises(ValueError):app.wifi_request('192.168.0.50','wifi_clear')
    def test_wifi_http_error_not_success(self):
        error=urllib.error.HTTPError('http://192.168.0.50/cmd',400,'Bad',{},io.BytesIO(b'JR_ERROR invalid'))
        with patch.object(app.urllib.request,'build_opener') as factory:
            factory.return_value.open.side_effect=error
            with self.assertRaisesRegex(RuntimeError,'recusou'):app.wifi_request('192.168.0.50','audio_volume 70')

class HTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server=app.ThreadingHTTPServer(('127.0.0.1',0),app.Handler)
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.url='http://127.0.0.1:'+str(cls.server.server_port)
    @classmethod
    def tearDownClass(cls):cls.server.shutdown();cls.server.server_close();cls.thread.join(1)
    def request(self,path,data=None,headers=None):
        req=urllib.request.Request(self.url+path,data=data,headers=headers or {})
        try:
            with urllib.request.urlopen(req,timeout=2) as r:return r.status,r.read().decode()
        except urllib.error.HTTPError as e:return e.code,e.read().decode()
    def test_root_and_script(self):
        code,body=self.request('/');self.assertEqual(code,200);self.assertIn('panel.js',body);self.assertNotIn('{APP_VERSION}',body)
        code,body=self.request('/panel.js');self.assertEqual(code,200);self.assertIn('serverCursor',body)
    def test_missing_csrf_header(self):self.assertEqual(self.request('/send',b'command=status')[0],403)
    def test_wrong_origin(self):self.assertEqual(self.request('/send',b'command=status',{'X-JrBot-Panel':'1','Origin':'http://evil.invalid'})[0],403)
    def test_invalid_content_length(self):self.assertEqual(self.request('/send',b'',{'X-JrBot-Panel':'1','Content-Length':'-1'})[0],413)
    def test_send_requires_real_confirmation(self):
        app.SERIAL=SerialMock('ok')
        data=urllib.parse.urlencode({'command':'audio_volume 70','mode':'serial'}).encode()
        code,body=self.request('/send',data,{'X-JrBot-Panel':'1'});self.assertEqual(code,200);self.assertIn('JR_OK',body)
    def test_send_error_http_code(self):
        app.SERIAL=SerialMock('no')
        data=urllib.parse.urlencode({'command':'audio_volume 70','mode':'serial'}).encode()
        code,body=self.request('/send',data,{'X-JrBot-Panel':'1'});self.assertEqual(code,502);self.assertIn('JR_ERROR',body)
    def test_camera_disabled(self):self.assertEqual(self.request('/camera/capture')[0],503)

class DiagnosticTests(unittest.TestCase):
    def setUp(self):
        app.SERIAL=None
        with app.PENDING_LOCK: app.PENDING.clear()
    def test_camera_wifi_rejected_before_request(self):
        with patch.object(app.urllib.request,'build_opener') as opener:
            with self.assertRaisesRegex(ValueError,'COM4'):app.wifi_request('192.168.0.50','camera_test')
            opener.assert_not_called()
    def test_camera_has_longer_ack_timeout(self):
        class TimeoutEvent:
            timeout=None
            def wait(self,timeout):self.timeout=timeout;return False
        event=TimeoutEvent();app.SERIAL=SerialMock('timeout')
        with patch.object(app.threading,'Event',return_value=event):
            with self.assertRaises(RuntimeError):app.serial_request('camera_test')
        self.assertEqual(event.timeout,30.0)
        self.assertFalse(app.PENDING)
    def test_standard_command_keeps_short_timeout(self):
        class TimeoutEvent:
            timeout=None
            def wait(self,timeout):self.timeout=timeout;return False
        event=TimeoutEvent();app.SERIAL=SerialMock('timeout')
        with patch.object(app.threading,'Event',return_value=event):
            with self.assertRaises(RuntimeError):app.serial_request('status')
        self.assertEqual(event.timeout,5.0)
    def test_camera_real_reply_forwarded(self):
        class CameraSerial(SerialMock):
            def write(self,b):
                ident=b.decode().split(' ',1)[0][1:]
                app.receive_line('JR_REPLY id='+ident+' ok=1 JR_OK camera_test=frame_received width=320 height=240 bytes=4000 released=1')
                return len(b)
        app.SERIAL=CameraSerial()
        self.assertIn('frame_received',app.serial_request('camera_test',.01))
    def test_camera_negative_reply_not_success(self):
        class CameraSerial(SerialMock):
            def write(self,b):
                ident=b.decode().split(' ',1)[0][1:]
                app.receive_line('JR_REPLY id='+ident+' ok=0 JR_ERROR camera_test=pins_not_confirmed')
                return len(b)
        app.SERIAL=CameraSerial()
        with self.assertRaisesRegex(RuntimeError,'pins_not_confirmed'):app.serial_request('camera_test',.01)

if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromModule(__import__(__name__))
    result=unittest.TextTestRunner(verbosity=2).run(suite)

    raise SystemExit(not result.wasSuccessful())
