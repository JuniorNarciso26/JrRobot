"""Read-only discovery of model-00 licenses; discovery never authorizes execution."""
import hashlib
import json
import struct

from solders.pubkey import Pubkey
import solana_skill
import wallet_auth
from wallet_purchase import _account, PROGRAM

MAX_LICENSES = 32


def _base58(raw):
    alphabet = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'
    value = int.from_bytes(raw, 'big')
    text = ''
    while value:
        value, digit = divmod(value, 58)
        text = alphabet[digit] + text
    return '1' * (len(raw) - len(raw.lstrip(b'\0'))) + text


def _slot(result, minimum=0):
    context = result.get('context') if isinstance(result, dict) else None
    slot = context.get('slot') if isinstance(context, dict) else None
    if type(slot) is not int or slot < minimum:
        raise ValueError('Slot da consulta de Skills invalido')
    return slot


def discover(address, rpc=None):
    rpc = rpc or solana_skill._rpc
    buyer = Pubkey.from_bytes(wallet_auth.public_key(address))
    if rpc('getGenesisHash', [], 5) != solana_skill.GENESIS:
        raise ValueError('Busca de Skills exige Solana Devnet')
    result = rpc('getProgramAccounts', [str(PROGRAM), {
        'encoding': 'base64', 'commitment': 'finalized', 'withContext': True,
        'filters': [{'dataSize': 137},
                    {'memcmp': {'offset': 0, 'bytes': _base58(hashlib.sha256(b'account:License').digest()[:8])}},
                    {'memcmp': {'offset': 9, 'bytes': address}}],
    }], 10)
    slot = _slot(result)
    rows = result.get('value')
    if not isinstance(rows, list) or len(rows) > MAX_LICENSES:
        raise ValueError('Busca excede o limite de 32 licencas ou resposta invalida')
    licenses, seen = [], set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('Registro de licenca invalido')
        raw = _account(row.get('account'), 'License', 137)
        skill = Pubkey.from_bytes(raw[41:73])
        offer = Pubkey.find_program_address([b'offer', b'00', bytes(skill)], PROGRAM)[0]
        pda = Pubkey.find_program_address([b'license', bytes(buyer), bytes(skill)], PROGRAM)[0]
        price, creator_paid, treasury_paid, issued = struct.unpack_from('<QQQq', raw, 105)
        if (raw[8] != 0 or raw[9:41] != bytes(buyer) or raw[73:105] != bytes(offer)
                or row.get('pubkey') != str(pda) or str(skill) in seen
                or not price or creator_paid + treasury_paid != price):
            raise ValueError('Licenca/PDA/comprador/termos invalidos')
        seen.add(str(skill))
        licenses.append({'skill': str(skill), 'offer': str(offer), 'license': str(pda),
                         'model_version': 0, 'price_paid': str(price), 'issued_at': issued})
    skills = []
    if licenses:
        keys = [key for item in licenses for key in (item['skill'], item['offer'])]
        accounts = rpc('getMultipleAccounts', [keys, {'encoding': 'base64', 'commitment': 'finalized',
                                                     'minContextSlot': slot}], 10)
        slot = _slot(accounts, slot)
        values = accounts.get('value')
        if not isinstance(values, list) or len(values) != len(keys):
            raise ValueError('Resposta de Skills/ofertas invalida')
        for index, license in enumerate(licenses):
            raw = _account(values[index * 2], 'Skill', 589)
            offer = _account(values[index * 2 + 1], 'Offer', 80)
            authority, digest = raw[8:40], raw[41:73]
            length = struct.unpack_from('<I', raw, 73)[0]
            if not 1 <= length <= 512 or raw[40] != 1:
                raise ValueError('Schema/tamanho da Skill invalido')
            payload = raw[77:77 + length]
            expected = Pubkey.find_program_address([b'skill', authority, digest], PROGRAM)[0]
            if (str(expected) != license['skill'] or hashlib.sha256(payload).digest() != digest
                    or offer[8:40] != bytes(expected) or offer[40:72] != authority
                    or struct.unpack_from('<Q', offer, 72)[0] != int(license['price_paid'])):
                raise ValueError('Skill/hash/oferta nao corresponde a licenca')
            doc = json.loads(payload.decode('utf-8', errors='strict'))
            if (not isinstance(doc, dict) or type(doc.get('v')) is not int or doc['v'] != 1
                    or not isinstance(doc.get('run'), list) or not 1 <= len(doc['run']) <= 64):
                raise ValueError('JSON JrSkill v1 invalido')
            checkpoint = (license['skill'] == solana_skill.PDA and authority == solana_skill.AUTHORITY_BYTES
                          and digest.hex() == solana_skill.EXPECTED_HASH and length == 128)
            skills.append(dict(license, name='minimal_recipe_01' if checkpoint else 'Skill ' + license['skill'][:8],
                               authority=str(Pubkey.from_bytes(authority)), payload_hash=digest.hex(),
                               payload_bytes=length, schema_version=1, hash_verified=True,
                               checkpoint_supported=checkpoint))
    return {'cluster': 'devnet', 'genesis': solana_skill.GENESIS, 'program': str(PROGRAM),
            'buyer': address, 'commitment': 'finalized', 'rpc_slot': slot,
            'skills': sorted(skills, key=lambda item: item['skill'])}
