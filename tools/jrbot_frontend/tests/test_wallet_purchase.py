import base64
import hashlib
import json
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import solana_skill
import wallet_purchase as purchase
from solders.hash import Hash
from solders.keypair import Keypair
from solders.pubkey import Pubkey
from solders.transaction import Transaction

ASSETS = Path(__file__).resolve().parents[3] / 'docs/HACKATHON_DEVLOG/assets/day6'


def value(kind, payload):
    raw = hashlib.sha256(('account:' + kind).encode()).digest()[:8] + payload
    return {'owner': solana_skill.PROGRAM, 'executable': False, 'data': [base64.b64encode(raw).decode(), 'base64']}


class FakeRPC:
    def __init__(self):
        import struct
        self.buyer = Keypair()
        self.creator = Pubkey.from_string(solana_skill.AUTHORITY)
        self.treasury = Pubkey.from_string('Ex9pYvpA96dkZF5efdooEU2pQvWr5owpAZPiqXtLYxnh')
        payload = (Path(__file__).resolve().parents[1] / 'jrskill/skills/minimal_recipe_01.json').read_bytes().replace(b'\r\n', b'\n')
        self.values = [value('MarketConfig', bytes(self.creator) + bytes(self.treasury) + struct.pack('<H', 5000)),
            value('Offer', bytes(purchase.SKILL) + bytes(self.creator) + struct.pack('<Q', 1000000000)),
            value('Skill', bytes(self.creator) + b'\1' + bytes.fromhex(solana_skill.EXPECTED_HASH) + struct.pack('<I', len(payload)) + payload + bytes(512 - len(payload))), None]
        self.calls = []; self.genesis = solana_skill.GENESIS; self.balance = 5000000000; self.height = 1; self.fail_send = False

    def __call__(self, method, params, timeout):
        self.calls.append((method, params))
        if method == 'getGenesisHash': return self.genesis
        if method == 'getMultipleAccounts': return {'context': {'slot': 1}, 'value': self.values}
        if method == 'getLatestBlockhash': return {'value': {'blockhash': str(Hash.new_unique()), 'lastValidBlockHeight': 100}}
        if method == 'getMinimumBalanceForRentExemption': return 1346200
        if method == 'getFeeForMessage': return {'value': 5000}
        if method == 'getBalance': return {'value': self.balance}
        if method == 'getBlockHeight': return self.height
        if method == 'sendTransaction':
            if self.fail_send: raise TimeoutError('ambiguous timeout')
            return str(Transaction.from_bytes(base64.b64decode(params[0])).signatures[0])
        raise AssertionError(method)


class Auth:
    def __init__(self, address): self.address = address
    def status(self, token, origin):
        return {'authenticated': bool(self.address) and token == 'token' and origin == 'origin', 'address': self.address}


class PurchaseTests(unittest.TestCase):
    def setUp(self):
        self.rpc = FakeRPC(); self.address = str(self.rpc.buyer.pubkey()); self.auth = Auth(self.address); self.now = 100
        self.store = purchase.PurchaseStore(self.rpc, self.auth, lambda: self.now)

    def quote(self): return self.store.quote('token', 'origin', self.address)

    def signed(self, quote, key=None, change=False):
        tx = Transaction.from_bytes(base64.b64decode(quote['transaction']))
        tx.sign([key or self.rpc.buyer], Hash.new_unique() if change else tx.message.recent_blockhash)
        return base64.b64encode(bytes(tx)).decode()

    def test_quote_and_signature_relay_have_only_expected_instruction_and_costs(self):
        quote = self.quote()
        self.assertEqual(quote['total_lamports'], '1001351200')
        tx = Transaction.from_bytes(base64.b64decode(quote['transaction']))
        self.assertEqual(len(tx.message.instructions), 1)
        self.assertEqual(tx.message.header.num_required_signatures, 1)
        self.assertEqual(tx.message.account_keys[0], self.rpc.buyer.pubkey())
        self.assertLessEqual(len(bytes(tx)), 1232)
        self.assertNotIn('sendTransaction', [item[0] for item in self.rpc.calls])
        signed = self.signed(quote)
        result = self.store.submit('token', 'origin', self.address, quote['quote_id'], signed)
        self.assertEqual(result['state'], 'submitted')
        self.assertEqual([method for method, _ in self.rpc.calls].count('sendTransaction'), 1)
        with self.assertRaises(ValueError): self.store.submit('token', 'origin', self.address, quote['quote_id'], signed)

    def test_mutation_unsigned_wrong_session_expiry_and_wrong_network_never_relay(self):
        quote = self.quote()
        for token, origin, data in [('other', 'origin', self.signed(quote)), ('token', 'other', self.signed(quote)),
                                    ('token', 'origin', quote['transaction']), ('token', 'origin', self.signed(quote, change=True))]:
            with self.assertRaises(ValueError): self.store.submit(token, origin, self.address, quote['quote_id'], data)
        self.now += purchase.QUOTE_TTL
        with self.assertRaises(ValueError): self.store.submit('token', 'origin', self.address, quote['quote_id'], self.signed(quote))
        self.rpc.genesis = 'mainnet'
        with self.assertRaises(ValueError): self.quote()
        self.assertNotIn('sendTransaction', [item[0] for item in self.rpc.calls])

    def test_expired_blockhash_balance_and_session_change_fail_closed(self):
        self.rpc.balance = 1
        with self.assertRaises(ValueError): self.quote()
        self.rpc.balance = 5000000000
        quote = self.quote(); self.rpc.height = 101
        with self.assertRaises(ValueError): self.store.submit('token', 'origin', self.address, quote['quote_id'], self.signed(quote))
        self.auth.address = ''
        with self.assertRaises(ValueError): self.quote()
        self.assertNotIn('sendTransaction', [item[0] for item in self.rpc.calls])

    def test_timeout_consumes_quote_and_returns_signature_without_retry(self):
        quote = self.quote(); self.rpc.fail_send = True
        signed = self.signed(quote)
        result = self.store.submit('token', 'origin', self.address, quote['quote_id'], signed)
        self.assertEqual(result['state'], 'unknown')
        self.assertTrue(result['signature'])
        self.assertIn('ambiguous timeout', result['error_detail'])
        with self.assertRaises(ValueError): self.store.submit('token', 'origin', self.address, quote['quote_id'], signed)
        self.assertEqual([method for method, _ in self.rpc.calls].count('sendTransaction'), 1)

    def test_rpc_diagnostic_preserves_simulation_error_without_request_bytes(self):
        import io
        from unittest.mock import patch
        body = json.dumps({'error': {'code': -32002, 'message': 'Transaction simulation failed',
            'data': {'logs': ['Program log: Error Code: Example', 'A' * 200]}}}).encode()
        with patch('urllib.request.urlopen', return_value=io.BytesIO(body)):
            with self.assertRaises(ValueError) as error:
                solana_skill._rpc('sendTransaction', ['SECRET_REQUEST'], 10)
        self.assertIn('code=-32002', str(error.exception))
        self.assertIn('Error Code: Example', str(error.exception))
        self.assertNotIn('SECRET_REQUEST', str(error.exception))
        self.assertNotIn('A' * 100, str(error.exception))

    def test_actual_license_fixture_and_tampered_account_identity_are_rejected(self):
        import struct
        # Public fields recorded by the finalized Devnet proof, no signer key needed.
        proof = json.loads((ASSETS / 'license-purchase-proof.json').read_text())
        address = proof['buyer']
        self.rpc.values[3] = value('License', b'\0' + bytes(Pubkey.from_string(address)) + bytes(purchase.SKILL) + bytes(purchase.OFFER)
            + struct.pack('<QQQq', 1000000000, 500000000, 500000000, 1))
        self.assertTrue(purchase.inspect(address, self.rpc)['owned'])
        with self.assertRaises(ValueError): purchase.inspect(self.address, self.rpc)
        self.rpc.values[3]['owner'] = '11111111111111111111111111111111'
        with self.assertRaises(ValueError): purchase.inspect(address, self.rpc)

    def test_rpc_logout_during_quote_never_stores_signable_transaction(self):
        original = self.store.rpc
        def revoked(method, params, timeout):
            result = original(method, params, timeout)
            if method == 'getMultipleAccounts': self.auth.address = ''
            return result
        self.store.rpc = revoked
        with self.assertRaises(ValueError): self.quote()
        self.assertEqual(self.store.quotes, {})

    def test_rejection_codes_and_public_message_comparison(self):
        from solders.message import Message
        from solders.instruction import Instruction
        quote = self.quote()
        with self.assertRaises(purchase.PurchaseRejected) as error:
            self.store.submit('token', 'origin', self.address, 'missing', self.signed(quote))
        self.assertEqual(error.exception.code, 'quote_missing')
        self.now = 191
        with self.assertRaises(purchase.PurchaseRejected) as error:
            self.store.submit('token', 'origin', self.address, quote['quote_id'], self.signed(quote))
        self.assertEqual(error.exception.code, 'quote_expired')
        self.assertLess(error.exception.diagnostics['remaining_seconds'], 0)
        self.now = 100
        quote = self.quote()
        tx = Transaction.from_bytes(base64.b64decode(quote['transaction']))
        message = Message.new_with_blockhash([purchase.instruction(self.address, quote),
            Instruction(Pubkey.from_string('ComputeBudget111111111111111111111111111111'), b'\x02\x01\x00\x00\x00', [])],
            self.rpc.buyer.pubkey(), tx.message.recent_blockhash)
        changed = Transaction.new_unsigned(message); changed.sign([self.rpc.buyer], message.recent_blockhash)
        with self.assertRaises(purchase.PurchaseRejected) as error:
            self.store.submit('token', 'origin', self.address, quote['quote_id'], base64.b64encode(bytes(changed)).decode())
        self.assertEqual(error.exception.code, 'message_changed')
        diag = error.exception.diagnostics
        self.assertIn('instructions', diag['changed_fields'])
        self.assertEqual(diag['actual']['instruction_count'], 2)
        self.assertEqual(diag['expected']['instruction_count'], 1)
        self.assertIn('ComputeBudget', diag['actual']['instructions'][1]['program'])
        self.assertEqual(diag['actual']['blockhash'], diag['expected']['blockhash'])
        self.assertTrue(diag['signature_verified']); self.assertFalse(diag['relay_attempted'])
        self.assertNotIn(quote['transaction'], json.dumps(diag))
        self.assertNotIn('token', diag); self.assertNotIn('origin', diag)
        self.assertNotIn('sendTransaction', [item[0] for item in self.rpc.calls])


class PurchaseHTTPTests(unittest.TestCase):
    def test_origin_cookie_authentication_and_unsigned_submission_are_enforced(self):
        import app
        import threading
        import urllib.request
        import urllib.error
        import urllib.parse
        rpc = FakeRPC()
        original = purchase.STORE
        purchase.STORE = purchase.PurchaseStore(rpc)
        server = app.ThreadingHTTPServer(('127.0.0.1', 0), app.Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
        origin = f'http://127.0.0.1:{server.server_port}'
        cookie = ''
        def request(operation, data=None, source=None):
            headers = {'Cookie': cookie, 'Origin': source or origin, 'X-JrBot-Panel': '1'}
            body = None if data is None else urllib.parse.urlencode(data).encode()
            with urllib.request.urlopen(urllib.request.Request(origin + '/jrskill/wallet/' + operation, data=body, headers=headers)) as response:
                return json.load(response), response.headers.get('Set-Cookie')
        try:
            for operation, data in [('purchase/status', None), ('purchase/quote', {})]:
                with self.assertRaises(urllib.error.HTTPError) as error: request(operation, data)
                self.assertEqual(error.exception.code, 401)
            challenge, header = request('challenge', {'address': str(rpc.buyer.pubkey())})
            cookie = header.split(';')[0]
            signature = base64.b64encode(bytes(rpc.buyer.sign_message(challenge['message'].encode()))).decode()
            request('verify', {'id': challenge['id'], 'signature': signature})
            with self.assertRaises(urllib.error.HTTPError) as error: request('purchase/quote', {}, 'http://evil.example')
            self.assertEqual(error.exception.code, 403)
            quote, _ = request('purchase/quote', {})
            self.assertEqual(quote['buyer'], str(rpc.buyer.pubkey()))
            with self.assertRaises(urllib.error.HTTPError) as error:
                request('purchase/submit', {'quote_id': quote['quote_id'], 'signed_transaction': quote['transaction']})
            self.assertEqual(error.exception.code, 400)
            with app.LOG_LOCK:
                diagnostics = [line['line'] for line in app.LOGS if 'stage=server_validation' in line['line']]
            self.assertTrue(any('Transacao/assinatura invalida' in line for line in diagnostics))
            self.assertTrue(all(quote['transaction'] not in line for line in diagnostics))
            request('logout', {})
            with self.assertRaises(urllib.error.HTTPError) as error: request('purchase/status')
            self.assertEqual(error.exception.code, 401)
            self.assertNotIn('sendTransaction', [item[0] for item in rpc.calls])
        finally:
            server.shutdown(); server.server_close(); thread.join(); purchase.STORE = original


if __name__ == '__main__': unittest.main()
