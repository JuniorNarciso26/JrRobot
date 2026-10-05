import json
import sys
import threading
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import app
import wallet_execution as execution
import wallet_auth
from test_wallet_purchase import Auth, FakeRPC
import solana_skill


class ExecutionTests(unittest.TestCase):
    def setUp(self):
        self.auth = Auth('buyer'); self.now = 100; self.owned = True; self.calls = []
        rpc = FakeRPC()
        self.proof = solana_skill.decode_account({'value': rpc.values[2], 'context': {'slot': 1}})
        self.store = execution.ExecutionStore(self.auth, lambda buyer: {'owned': self.owned, 'license': 'license'}, lambda: self.proof, lambda: self.now)
    def start(self): return self.store.start('token', 'origin')
    def send(self, proof, command):
        return self.store.send('token', 'origin', proof['execution_permit'], 'api '+json.dumps(command), lambda cmd: self.calls.append(cmd) or 'ok')

    def test_authorized_sequence_is_consumed_and_replay_is_blocked(self):
        proof = self.start()
        sequence = execution.commands_for(proof)
        self.assertEqual([cmd['args']['expression'] for cmd in sequence[1:]], ['happy','surprised','thinking','happy','neutral'])
        for command in sequence: self.assertEqual(self.send(proof, command), 'ok')
        with self.assertRaises(ValueError): self.send(proof, sequence[-1])
        self.assertEqual(len(self.calls), 6)

    def test_absent_license_unauthenticated_and_offline_start_never_dispatch(self):
        self.owned = False
        with self.assertRaisesRegex(ValueError, 'license_absent'): self.start()
        self.assertEqual(self.store.runs, {})
        self.auth.address = ''
        with self.assertRaisesRegex(ValueError, 'auth_required'): self.start()
        self.auth.address = 'buyer'; self.owned = True
        self.store.inspect = lambda buyer: (_ for _ in ()).throw(TimeoutError('offline'))
        with self.assertRaises(TimeoutError): self.start()
        self.assertEqual(self.calls, [])

    def test_logout_expiry_wrong_session_sequence_and_midrun_offline_stop(self):
        for reason in ['logout','expiry','command','offline','license','account']:
            with self.subTest(reason=reason):
                self.setUp(); proof = self.start(); command = execution.commands_for(proof)[0]
                if reason == 'logout': self.auth.address = ''
                if reason == 'account': self.auth.address = 'other'
                if reason == 'expiry': self.now += execution.TTL
                if reason == 'command': command = {'v':1,'fn':'face','args':{'expression':'happy'}}
                if reason == 'license': self.owned = False
                if reason == 'offline': self.store.inspect = lambda buyer: (_ for _ in ()).throw(TimeoutError('offline'))
                with self.assertRaises((ValueError, TimeoutError)): self.send(proof, command)
                self.assertEqual(self.calls, [])
        self.setUp(); proof = self.start()
        with self.assertRaises(ValueError): self.store.send('wrong','origin',proof['execution_permit'],'api {}',lambda cmd:self.calls.append(cmd))
        self.assertEqual(self.calls, [])

    def test_late_rpc_account_change_and_new_run_invalidate_previous_authorization(self):
        proof = self.start()
        def changed(buyer): self.auth.address = 'other'; return {'owned':True,'license':'license'}
        self.store.inspect = changed
        with self.assertRaisesRegex(ValueError,'auth_required'): self.send(proof, execution.commands_for(proof)[0])
        self.assertEqual(self.calls, [])
        self.setUp(); old = self.start(); self.start()
        with self.assertRaises(ValueError): self.send(old, execution.commands_for(old)[0])

    def test_http_gate_checks_origin_session_license_before_serial_io(self):
        import base64
        import urllib.request, urllib.parse, urllib.error
        rpc = FakeRPC(); owned = False
        store = execution.ExecutionStore(inspect=lambda buyer:{'owned':owned,'license':'license'}, load=lambda:self.proof)
        server = app.ThreadingHTTPServer(('127.0.0.1',0),app.Handler)
        thread = threading.Thread(target=server.serve_forever,daemon=True); thread.start()
        origin = f'http://127.0.0.1:{server.server_port}'; cookie = ''
        def request(route, data, source=None):
            req = urllib.request.Request(origin+'/jrskill/wallet/'+route,
                data=urllib.parse.urlencode(data).encode(), headers={'Origin':source or origin,'Cookie':cookie,'X-JrBot-Panel':'1'})
            with urllib.request.urlopen(req) as response: return json.load(response),response.headers.get('Set-Cookie')
        try:
            with patch.object(execution,'STORE',store), patch.object(app,'serial_request',return_value='JR_API {"ok":true}') as serial:
                with self.assertRaises(urllib.error.HTTPError) as error: request('execution/start',{})
                self.assertEqual(error.exception.code,401)
                challenge,header = request('challenge',{'address':str(rpc.buyer.pubkey())});cookie=header.split(';')[0]
                request('verify',{'id':challenge['id'],'signature':base64.b64encode(bytes(rpc.buyer.sign_message(challenge['message'].encode()))).decode()})
                with self.assertRaises(urllib.error.HTTPError): request('execution/start',{})
                serial.assert_not_called()
                owned=True; proof,_=request('execution/start',{})
                command='api '+json.dumps(execution.commands_for(proof)[0])
                with self.assertRaises(urllib.error.HTTPError) as error: request('execution/send',{'permit':proof['execution_permit'],'command':command},'http://evil.example')
                self.assertEqual(error.exception.code,403);serial.assert_not_called()
                request('execution/send',{'permit':proof['execution_permit'],'command':command});serial.assert_called_once()
                request('logout',{})
                with self.assertRaises(urllib.error.HTTPError): request('execution/send',{'permit':proof['execution_permit'],'command':command})
                serial.assert_called_once()
        finally:
            server.shutdown();server.server_close();thread.join()


if __name__ == '__main__': unittest.main()
