import base64
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
import threading
import urllib.request
import urllib.error

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import app
import wallet_skills as search
from solders.pubkey import Pubkey


def account(kind, raw):
    return {'owner': str(search.PROGRAM), 'executable': False,
            'data': [base64.b64encode(hashlib.sha256(('account:' + kind).encode()).digest()[:8] + raw).decode(), 'base64']}


class RPC:
    def __init__(self):
        self.buyer = Pubkey.new_unique()
        self.rows, self.values, self.calls = [], [], []
        self.genesis = search.solana_skill.GENESIS
        for index in range(2):
            authority = Pubkey.new_unique()
            payload = json.dumps({'v': 1, 'run': [['face', 'happy'], ['wait', index]]}).encode()
            digest = hashlib.sha256(payload).digest()
            skill = Pubkey.find_program_address([b'skill', bytes(authority), digest], search.PROGRAM)[0]
            offer = Pubkey.find_program_address([b'offer', b'00', bytes(skill)], search.PROGRAM)[0]
            license = Pubkey.find_program_address([b'license', bytes(self.buyer), bytes(skill)], search.PROGRAM)[0]
            self.rows.append({'pubkey': str(license), 'account': account('License', b'\0' + bytes(self.buyer) + bytes(skill) + bytes(offer) + struct.pack('<QQQq', 100, 50, 50, 1))})
            self.values.extend([account('Skill', bytes(authority) + b'\1' + digest + struct.pack('<I', len(payload)) + payload + bytes(512 - len(payload))),
                                account('Offer', bytes(skill) + bytes(authority) + struct.pack('<Q', 100))])

    def __call__(self, method, params, timeout):
        self.calls.append((method, params))
        if method == 'getGenesisHash': return self.genesis
        if method == 'getProgramAccounts': return {'context': {'slot': 10}, 'value': self.rows}
        if method == 'getMultipleAccounts': return {'context': {'slot': 11}, 'value': self.values}
        raise AssertionError(method)


class SkillsTests(unittest.TestCase):
    def test_discovers_multiple_skills_with_buyer_filter_and_validated_links(self):
        rpc = RPC(); result = search.discover(str(rpc.buyer), rpc)
        self.assertEqual(len(result['skills']), 2)
        self.assertTrue(all(item['hash_verified'] and not item['checkpoint_supported'] for item in result['skills']))
        self.assertIn({'memcmp': {'offset': 9, 'bytes': str(rpc.buyer)}}, rpc.calls[1][1][1]['filters'])
        self.assertEqual(rpc.calls[2][1][1]['minContextSlot'], 10)

    def test_empty_is_success_but_wrong_network_and_rpc_failure_are_not_empty(self):
        rpc = RPC(); rpc.rows = []
        self.assertEqual(search.discover(str(rpc.buyer), rpc)['skills'], [])
        rpc.genesis = 'mainnet'
        with self.assertRaises(ValueError): search.discover(str(rpc.buyer), rpc)
        with self.assertRaises(TimeoutError): search.discover(str(rpc.buyer), lambda *_: (_ for _ in ()).throw(TimeoutError()))

    def test_rejects_other_buyer_forged_pda_duplicates_and_limit(self):
        for mutation in ('buyer', 'pda', 'duplicate', 'limit'):
            rpc = RPC()
            if mutation == 'buyer': rpc.buyer = Pubkey.new_unique()
            if mutation == 'pda': rpc.rows[0]['pubkey'] = str(Pubkey.new_unique())
            if mutation == 'duplicate': rpc.rows[1] = rpc.rows[0]
            if mutation == 'limit': rpc.rows *= 17
            with self.subTest(mutation=mutation), self.assertRaises(ValueError): search.discover(str(rpc.buyer), rpc)

    def test_rejects_tampered_skill_hash_owner_offer_or_slot(self):
        for mutation in ('hash', 'owner', 'offer', 'slot'):
            rpc = RPC()
            if mutation == 'owner': rpc.values[0]['owner'] = str(Pubkey.default())
            if mutation in ('hash', 'offer'):
                index = 0 if mutation == 'hash' else 1
                raw = bytearray(base64.b64decode(rpc.values[index]['data'][0])); raw[77 if index == 0 else 8] ^= 1
                rpc.values[index]['data'][0] = base64.b64encode(raw).decode()
            call = rpc
            if mutation == 'slot':
                def call(method, params, timeout):
                    result = rpc(method, params, timeout)
                    if method == 'getMultipleAccounts': result['context']['slot'] = 9
                    return result
            with self.subTest(mutation=mutation), self.assertRaises(ValueError): search.discover(str(rpc.buyer), call)

    def test_http_requires_auth_and_rechecks_after_lookup(self):
        import run_panel_network
        server = app.ThreadingHTTPServer(('127.0.0.1', 0), app.Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
        url = f'http://127.0.0.1:{server.server_port}/jrskill/wallet/skills'
        rpc = RPC(); address = str(rpc.buyer)
        try:
            with patch.object(app.wallet_auth.STORE, 'status', return_value={'authenticated': False}), patch.object(search, 'discover') as discover:
                with self.assertRaises(urllib.error.HTTPError) as error: urllib.request.urlopen(url)
                self.assertEqual(error.exception.code, 401); discover.assert_not_called()
            for after in ({'authenticated': False}, {'authenticated': True, 'address': str(Pubkey.new_unique())}):
                with patch.object(app.wallet_auth.STORE, 'status', side_effect=[{'authenticated': True, 'address': address}, after]), patch.object(search, 'discover', return_value={'skills': [], 'rpc_slot': 10}):
                    with self.assertRaises(urllib.error.HTTPError) as error: urllib.request.urlopen(url)
                    self.assertEqual(error.exception.code, 401)
            search_result = search.discover(address, rpc)
            with patch.object(app.wallet_auth.STORE, 'status', return_value={'authenticated': True, 'address': address}), patch.object(search, 'discover', return_value=search_result):
                with urllib.request.urlopen(url) as response: self.assertEqual(len(json.load(response)['skills']), 2)
        finally:
            server.shutdown(); server.server_close(); thread.join()


if __name__ == '__main__': unittest.main()
