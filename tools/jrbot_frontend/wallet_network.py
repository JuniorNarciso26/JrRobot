"""Read-only confirmation of the panel's Devnet RPC and authenticated wallet balance."""
import solana_skill
import wallet_auth


def inspect(address, rpc=None):
    wallet_auth.public_key(address)
    rpc = rpc or solana_skill._rpc
    genesis = rpc('getGenesisHash', [], 5)
    if genesis != solana_skill.GENESIS:
        raise ValueError('RPC nao e Solana Devnet; consulta recusada')
    balance = rpc('getBalance', [address, {'commitment': 'finalized'}], 5)
    lamports = balance.get('value') if isinstance(balance, dict) else None
    slot = balance.get('context', {}).get('slot') if isinstance(balance, dict) else None
    if type(lamports) is not int or not 0 <= lamports <= 2**64 - 1 or type(slot) is not int or slot < 0:
        raise ValueError('Saldo/slot RPC invalido')
    sol = f'{lamports // 1_000_000_000}.{lamports % 1_000_000_000:09d}'.rstrip('0').rstrip('.')
    return {'cluster': 'devnet', 'rpc_verified': True, 'genesis': genesis,
            'address': address, 'balance_lamports': str(lamports), 'balance_sol': sol,
            'rpc_slot': slot, 'commitment': 'finalized'}
