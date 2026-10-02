from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import wallet_network
import solana_skill

ADDRESS = '6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R'


class NetworkTests(unittest.TestCase):
    def test_finalized_balance_and_exact_decimal_format(self):
        for lamports, sol in ((0, '0'), (1, '0.000000001'), (1_000_000_000, '1'), (1_500_000_000, '1.5'), (2**64-1, '18446744073.709551615')):
            calls = []
            def rpc(method, params, timeout):
                calls.append((method, params))
                return solana_skill.GENESIS if method == 'getGenesisHash' else {'context': {'slot': 123}, 'value': lamports}
            result = wallet_network.inspect(ADDRESS, rpc)
            self.assertEqual(result['balance_sol'], sol)
            self.assertEqual(result['balance_lamports'], str(lamports))
            self.assertEqual(calls[1][1], [ADDRESS, {'commitment': 'finalized'}])

    def test_wrong_genesis_never_queries_balance(self):
        calls = []
        def rpc(method, params, timeout):
            calls.append(method)
            return 'mainnet'
        with self.assertRaises(ValueError):
            wallet_network.inspect(ADDRESS, rpc)
        self.assertEqual(calls, ['getGenesisHash'])

    def test_invalid_balance_and_offline_are_not_zero(self):
        for invalid in (-1, True, '0', None, 2**64):
            def rpc(method, params, timeout):
                return solana_skill.GENESIS if method == 'getGenesisHash' else {'context': {'slot': 1}, 'value': invalid}
            with self.assertRaises(ValueError):
                wallet_network.inspect(ADDRESS, rpc)
        with self.assertRaises(OSError):
            wallet_network.inspect(ADDRESS, lambda *args: (_ for _ in ()).throw(OSError('offline')))


if __name__ == '__main__':
    unittest.main()
