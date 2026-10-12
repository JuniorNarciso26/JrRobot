"""Read-only Devnet Skill adapter; validates schema, hash and derived PDA."""
import base64
import hashlib
import json
import re
import struct
import time
import urllib.request

RPC = "https://api.devnet.solana.com"
GENESIS = "EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG"
PROGRAM = "Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454"
PDA = "8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux"
AUTHORITY = "3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij"
AUTHORITY_BYTES = bytes.fromhex("244730b82bf46825305434825d7413b132296edc9145b2374ab278f9bdb5eef4")
EXPECTED_HASH = "416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f"


def rpc_error_detail(error):
    """Bounded public diagnostic; never include a request or signed transaction."""
    text = str(error).replace('\r', ' ').replace('\n', ' ').replace('\t', ' ')
    return re.sub(r'[A-Za-z0-9+/=_-]{100,}', '[dados omitidos]', text)[:1200]


def _rpc(method, params, timeout):
    request = urllib.request.Request(RPC, data=json.dumps({
        "jsonrpc": "2.0", "id": 1, "method": method, "params": params,
    }).encode("utf-8"), headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        raw = response.read(65537)
    if len(raw) > 65536:
        raise ValueError("Resposta RPC excede o limite")
    body = json.loads(raw)
    if body.get("error"):
        error = body['error']
        if isinstance(error, dict):
            data = error.get('data')
            logs = data.get('logs', []) if isinstance(data, dict) else []
            log_text = ' | '.join(str(line) for line in logs[:8]) if isinstance(logs, list) else ''
            raise ValueError(rpc_error_detail('RPC code=' + str(error.get('code')) + ' message=' + str(error.get('message', 'erro')) + ' logs=' + log_text))
        raise ValueError('RPC devolveu erro sem detalhe valido')
    if "result" not in body:
        raise ValueError("RPC nao devolveu um resultado valido")
    return body["result"]


def decode_account(result, pda=PDA):
    account = result.get("value")
    if not account or account.get("owner") != PROGRAM or account.get("executable") is not False:
        raise ValueError("Skill ausente ou owner/tipo incorreto")
    encoded = account.get("data")
    if not isinstance(encoded, list) or len(encoded) != 2 or encoded[1] != "base64":
        raise ValueError("Codificacao RPC invalida")
    data = base64.b64decode(encoded[0], validate=True)
    if len(data) != 589 or data[:8] != hashlib.sha256(b"account:Skill").digest()[:8]:
        raise ValueError("Tamanho/discriminator Skill invalido")
    if data[40] != 1 or (pda == PDA and data[8:40] != AUTHORITY_BYTES):
        raise ValueError("Authority/schema invalido")
    length = struct.unpack_from("<I", data, 73)[0]
    if not 1 <= length <= 512 or 77 + length > len(data):
        raise ValueError("Tamanho do payload invalido")
    payload = data[77:77 + length]
    digest = hashlib.sha256(payload).digest()
    if digest != data[41:73] or (pda == PDA and (digest.hex() != EXPECTED_HASH or length != 128)):
        raise ValueError("Hash/payload invalido ou referencia divergente")
    from solders.pubkey import Pubkey
    expected = Pubkey.find_program_address([b'skill', data[8:40], digest], Pubkey.from_string(PROGRAM))[0]
    if str(expected) != pda:
        raise ValueError('Skill PDA nao corresponde ao autor/hash')
    text = payload.decode("utf-8", errors="strict")
    skill = json.loads(text)
    if not isinstance(skill, dict) or type(skill.get("v")) is not int or skill["v"] != 1 or not isinstance(skill.get("run"), list) or not 1 <= len(skill["run"]) <= 64:
        raise ValueError("JSON JrSkill v1 invalido")
    slot = result.get("context", {}).get("slot")
    if type(slot) is not int or slot < 0:
        raise ValueError("Slot RPC invalido")
    return {"source": "solana-devnet", "commitment": "finalized", "rpc_slot": slot,
            "program": PROGRAM, "pda": pda, "authority": str(Pubkey.from_bytes(data[8:40])), "schema_version": 1,
            "payload_bytes": length, "payload_hash": digest.hex(), "hash_verified": True,
            "matches_checkpoint": pda == PDA, "payload_text": text}


def check_connection(rpc=None):
    """Probe the fixed RPC/cluster; this does not cache or validate a Skill."""
    rpc = rpc or _rpc
    if rpc("getGenesisHash", [], 5) != GENESIS:
        raise ValueError("Devnet obrigatoria: genesis incorreto")
    return {"available": True, "cluster": "devnet"}


def load_skill(rpc=None, pda=PDA):
    # A fresh read per invocation; no local Skill file, cache, wallet or fallback.
    rpc = rpc or _rpc
    from solders.pubkey import Pubkey
    Pubkey.from_string(pda)
    deadline = time.monotonic() + 25
    if rpc("getGenesisHash", [], 10) != GENESIS:
        raise ValueError("Devnet obrigatoria: genesis incorreto")
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise TimeoutError("Tempo da consulta Devnet esgotado")
    result = rpc("getAccountInfo", [pda, {"encoding": "base64", "commitment": "finalized"}], min(15, remaining))
    return decode_account(result, pda)


if __name__ == "__main__":
    print(json.dumps(load_skill(), ensure_ascii=False, indent=2))
