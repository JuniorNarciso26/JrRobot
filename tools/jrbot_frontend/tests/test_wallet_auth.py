import base64
from http.cookiejar import CookieJar
from pathlib import Path
import sys
import threading
import unittest
import urllib.error
import urllib.parse
import urllib.request

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import wallet_auth
import run_panel_network
import app


def address(key):
    raw = key.public_key().public_bytes_raw()
    number = int.from_bytes(raw, 'big')
    result = ''
    while number:
        number, remainder = divmod(number, 58)
        result = wallet_auth._ALPHABET[remainder] + result
    return '1' * (len(raw) - len(raw.lstrip(b'\0'))) + result


class AuthTests(unittest.TestCase):
    def setUp(self):
        self.now = 1000
        self.store = wallet_auth.AuthStore(lambda: self.now)
        self.key = Ed25519PrivateKey.generate()
        self.address = address(self.key)
        self.origin = 'http://127.0.0.1:8765'

    def challenge(self):
        return self.store.challenge('', self.origin, self.address)

    def sign(self, challenge, key=None):
        return base64.b64encode((key or self.key).sign(challenge['message'].encode())).decode()

    def test_signature_authenticates_exact_address_and_session_expires(self):
        token, challenge = self.challenge()
        self.assertIn(self.origin, challenge['message'])
        self.assertIn(self.address, challenge['message'])
        result = self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))
        self.assertEqual(result['address'], self.address)
        self.assertTrue(self.store.status(token, self.origin)['authenticated'])
        self.now += wallet_auth.SESSION_TTL
        self.assertFalse(self.store.status(token, self.origin)['authenticated'])

    def test_wrong_key_and_tampered_message_fail_and_consume_challenge(self):
        for signature_type in ('other-key', 'altered-message', 'malformed'):
            token, challenge = self.challenge()
            signature = self.sign(challenge, Ed25519PrivateKey.generate()) if signature_type == 'other-key' else (
                base64.b64encode(self.key.sign(b'altered')).decode() if signature_type == 'altered-message' else 'not-base64')
            with self.assertRaises(ValueError):
                self.store.verify(token, self.origin, challenge['id'], signature)
            self.assertFalse(self.store.status(token, self.origin)['authenticated'])
            with self.assertRaises(ValueError):
                self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))

    def test_replay_expired_challenge_and_cross_session_are_rejected(self):
        token, challenge = self.challenge()
        other, _ = self.challenge()
        with self.assertRaises(ValueError):
            self.store.verify(other, self.origin, challenge['id'], self.sign(challenge))
        with self.assertRaises(ValueError):
            self.store.verify(token, 'http://localhost:8765', challenge['id'], self.sign(challenge))
        self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))
        with self.assertRaises(ValueError):
            self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))
        token, challenge = self.challenge()
        self.now += wallet_auth.CHALLENGE_TTL
        with self.assertRaises(ValueError):
            self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))

    def test_logout_replacement_and_address_validation(self):
        token, challenge = self.challenge()
        _, replacement = self.store.challenge(token, self.origin, self.address)
        with self.assertRaises(ValueError):
            self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))
        token, challenge = self.challenge()
        self.store.logout(token)
        with self.assertRaises(ValueError):
            self.store.verify(token, self.origin, challenge['id'], self.sign(challenge))
        for invalid in ('', '0' * 32, '1' * 33, '<script>', self.address + '\n'):
            with self.assertRaises(ValueError):
                self.store.challenge('', self.origin, invalid)
        self.assertEqual(wallet_auth.public_key('1' * 32), bytes(32))

    def test_store_bound_and_cookie_parser(self):
        for _ in range(wallet_auth.MAX_SESSIONS):
            self.challenge()
        with self.assertRaises(RuntimeError):
            self.challenge()
        self.now += wallet_auth.SESSION_TTL
        token, _ = self.challenge()
        self.assertEqual(wallet_auth.cookie_token('jr_wallet_session=' + token), token)
        self.assertEqual(wallet_auth.cookie_token('jr_wallet_session=invalid'), '')


class AuthHttpTests(unittest.TestCase):
    def test_real_http_cookie_origin_verify_status_logout(self):
        server = app.ThreadingHTTPServer(('127.0.0.1', 0), app.Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        origin = f'http://127.0.0.1:{server.server_port}'
        client = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(CookieJar()))
        def request(operation, data=None, include_origin=True, panel=True):
            headers = {'X-JrBot-Panel': '1'} if panel else {}
            if include_origin: headers['Origin'] = origin
            req = urllib.request.Request(origin + '/jrskill/wallet/' + operation,
                data=urllib.parse.urlencode(data).encode() if data is not None else None, headers=headers)
            return client.open(req)
        key = Ed25519PrivateKey.generate()
        try:
            for origin_header, panel_header in ((False, True), (True, False)):
                with self.assertRaises(urllib.error.HTTPError) as rejected:
                    request('challenge', {'address': address(key)}, origin_header, panel_header)
                self.assertEqual(rejected.exception.code, 403)
            with request('challenge', {'address': address(key)}) as response:
                import json
                challenge = json.load(response)
                cookie = response.headers['Set-Cookie']
                self.assertIn('HttpOnly', cookie)
                self.assertIn('SameSite=Strict', cookie)
            signature = base64.b64encode(key.sign(challenge['message'].encode())).decode()
            with request('verify', {'id': challenge['id'], 'signature': signature}) as response:
                self.assertTrue(json.load(response)['authenticated'])
            with request('status') as response:
                self.assertEqual(json.load(response)['address'], address(key))
            with request('logout', {}) as response:
                self.assertFalse(json.load(response)['authenticated'])
            with request('status') as response:
                self.assertFalse(json.load(response)['authenticated'])
        finally:
            server.shutdown(); server.server_close(); thread.join()


if __name__ == '__main__':
    unittest.main()
