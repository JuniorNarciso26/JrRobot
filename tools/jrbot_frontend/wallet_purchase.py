"""One checkpoint Skill, Devnet only. No server-side keys or arbitrary transactions."""
import base64
import hashlib
import secrets
import struct
import threading
import time

import solana_skill
import wallet_auth
from solders.hash import Hash
from solders.instruction import AccountMeta, Instruction
from solders.message import Message
from solders.pubkey import Pubkey
from solders.transaction import Transaction

PROGRAM = Pubkey.from_string(solana_skill.PROGRAM)
SKILL = Pubkey.from_string(solana_skill.PDA)
SYSTEM = Pubkey.default()
MARKET = Pubkey.find_program_address([b'market', b'00'], PROGRAM)[0]
OFFER = Pubkey.find_program_address([b'offer', b'00', bytes(SKILL)], PROGRAM)[0]
QUOTE_TTL = 90


def license_address(buyer):
    return Pubkey.find_program_address([b'license', bytes(buyer), bytes(SKILL)], PROGRAM)[0]


def _account(value, kind, size):
    if not isinstance(value, dict) or value.get('owner') != str(PROGRAM) or value.get('executable') is not False:
        raise ValueError('Conta comercial ausente ou owner/tipo invalido')
    encoded = value.get('data')
    if not isinstance(encoded, list) or len(encoded) != 2 or encoded[1] != 'base64':
        raise ValueError('Codificacao da conta invalida')
    raw = base64.b64decode(encoded[0], validate=True)
    if len(raw) != size or raw[:8] != hashlib.sha256(('account:' + kind).encode()).digest()[:8]:
        raise ValueError('Tamanho/discriminator comercial invalido')
    return raw


def _integer(value):
    if type(value) is not int or not 0 <= value <= 2**64 - 1:
        raise ValueError('Lamports/altura RPC invalida')
    return value


def inspect(address, rpc=None):
    rpc = rpc or solana_skill._rpc
    buyer = Pubkey.from_bytes(wallet_auth.public_key(address))
    if rpc('getGenesisHash', [], 5) != solana_skill.GENESIS:
        raise ValueError('Compra exige RPC Solana Devnet')
    license_pda = license_address(buyer)
    result = rpc('getMultipleAccounts', [[str(MARKET), str(OFFER), str(SKILL), str(license_pda)],
                 {'encoding': 'base64', 'commitment': 'finalized'}], 10)
    values = result.get('value') if isinstance(result, dict) else None
    if not isinstance(values, list) or len(values) != 4:
        raise ValueError('Resposta de contas invalida')
    market = _account(values[0], 'MarketConfig', 74)
    offer = _account(values[1], 'Offer', 80)
    solana_skill.decode_account({'value': values[2], 'context': result.get('context', {})})
    treasury = Pubkey.from_bytes(market[40:72])
    fee_bps = struct.unpack_from('<H', market, 72)[0]
    creator = Pubkey.from_bytes(offer[40:72])
    price = struct.unpack_from('<Q', offer, 72)[0]
    if offer[8:40] != bytes(SKILL) or bytes(creator) != solana_skill.AUTHORITY_BYTES or not price or fee_bps > 10000 or treasury == SYSTEM:
        raise ValueError('Oferta/configuracao nao corresponde a Skill')
    fee = price * fee_bps // 10000
    owned = values[3] is not None
    if owned:
        license_data = _account(values[3], 'License', 137)
        if (license_data[8] != 0 or license_data[9:41] != bytes(buyer) or license_data[41:73] != bytes(SKILL)
                or license_data[73:105] != bytes(OFFER) or struct.unpack_from('<QQQ', license_data, 105) != (price, price - fee, fee)):
            raise ValueError('Licenca nao corresponde ao comprador/Skill/termos')
    return {'cluster': 'devnet', 'genesis': solana_skill.GENESIS, 'buyer': address,
            'program': str(PROGRAM), 'skill': str(SKILL), 'offer': str(OFFER),
            'license': str(license_pda), 'owned': owned, 'commitment': 'finalized',
            'price_lamports': str(price), 'creator': str(creator), 'creator_lamports': str(price - fee),
            'treasury': str(treasury), 'treasury_lamports': str(fee), 'model_version': 0}


def instruction(buyer, terms):
    buyer = Pubkey.from_string(buyer)
    accounts = [(MARKET, False, False), (SKILL, False, False), (OFFER, False, False),
                (license_address(buyer), False, True), (buyer, True, True),
                (Pubkey.from_string(terms['creator']), False, True),
                (Pubkey.from_string(terms['treasury']), False, True), (SYSTEM, False, False)]
    data = hashlib.sha256(b'global:buy_license').digest()[:8] + struct.pack('<Q', int(terms['price_lamports']))
    return Instruction(PROGRAM, data, [AccountMeta(*item) for item in accounts])


class PurchaseStore:
    def __init__(self, rpc=None, auth=None, clock=time.time):
        self.rpc = rpc or solana_skill._rpc
        self.auth = auth or wallet_auth.STORE
        self.clock = clock
        self.lock = threading.Lock()
        self.quotes = {}

    def authorized(self, token, origin, address):
        status = self.auth.status(token, origin)
        if not status.get('authenticated') or status.get('address') != address:
            raise ValueError('Sessao expirada/alterada; autentique novamente')

    def quote(self, token, origin, address):
        self.authorized(token, origin, address)
        terms = inspect(address, self.rpc)
        if terms['owned']:
            return terms
        if address in (terms['creator'], terms['treasury']):
            raise ValueError('Comprador deve ser diferente dos recebedores')
        block = self.rpc('getLatestBlockhash', [{'commitment': 'confirmed'}], 5)['value']
        height = _integer(block['lastValidBlockHeight'])
        message = Message.new_with_blockhash([instruction(address, terms)], Pubkey.from_string(address), Hash.from_string(block['blockhash']))
        tx = Transaction.new_unsigned(message)
        rent = _integer(self.rpc('getMinimumBalanceForRentExemption', [137, {'commitment': 'confirmed'}], 5))
        fee = _integer(self.rpc('getFeeForMessage', [base64.b64encode(bytes(message)).decode(), {'commitment': 'confirmed'}], 5)['value'])
        balance = _integer(self.rpc('getBalance', [address, {'commitment': 'finalized'}], 5)['value'])
        total = int(terms['price_lamports']) + rent + fee
        if balance < total:
            raise ValueError('Saldo Devnet insuficiente para preco, deposito e taxa')
        self.authorized(token, origin, address)
        quote_id = secrets.token_urlsafe(24)
        expires = self.clock() + QUOTE_TTL
        with self.lock:
            self.quotes = {key: value for key, value in self.quotes.items() if value['expires'] > self.clock() and value['token'] != token}
            if len(self.quotes) >= 256:
                raise ValueError('Limite de cotacoes atingido')
            self.quotes[quote_id] = dict(token=token, origin=origin, address=address,
                expires=expires, height=height, message=bytes(message))
        return dict(terms, quote_id=quote_id, expires_at=expires, transaction=base64.b64encode(bytes(tx)).decode(),
                    rent_lamports=str(rent), fee_lamports=str(fee), total_lamports=str(total))

    def submit(self, token, origin, address, quote_id, encoded):
        self.authorized(token, origin, address)
        if not isinstance(encoded, str) or len(encoded) > 1644:
            raise ValueError('Transacao assinada invalida')
        try:
            raw = base64.b64decode(encoded, validate=True)
            tx = Transaction.from_bytes(raw)
            tx.sanitize()
            tx.verify()
        except Exception as exc:
            raise ValueError('Transacao/assinatura invalida') from exc
        with self.lock:
            quote = self.quotes.get(quote_id)
            if (not quote or quote['token'] != token or quote['origin'] != origin or quote['address'] != address
                    or quote['expires'] <= self.clock() or tx.message_data() != quote['message'] or bytes(tx) != raw):
                raise ValueError('Cotacao usada/expirada ou transacao alterada')
        if inspect(address, self.rpc)['owned']:
            return {'cluster': 'devnet', 'buyer': address, 'state': 'owned', 'already_exists': True}
        if _integer(self.rpc('getBlockHeight', [{'commitment': 'confirmed'}], 5)) > quote['height']:
            raise ValueError('Blockhash expirou; consulte nova cotacao')
        self.authorized(token, origin, address)
        with self.lock:
            if self.quotes.pop(quote_id, None) is not quote:
                raise ValueError('Cotacao ja utilizada')
        # Consume before relay. A timeout can mean submitted; never auto-resend.
        signature = str(tx.signatures[0])
        state = 'submitted'
        try:
            sent = self.rpc('sendTransaction', [encoded, {'encoding': 'base64', 'skipPreflight': False,
                       'preflightCommitment': 'confirmed', 'maxRetries': 0}], 10)
            if sent != signature:
                state = 'unknown'
        except (ValueError, TimeoutError, OSError):
            state = 'unknown'
        return {'cluster': 'devnet', 'buyer': address, 'signature': signature, 'state': state,
                'license': str(license_address(Pubkey.from_string(address)))}


STORE = PurchaseStore()
