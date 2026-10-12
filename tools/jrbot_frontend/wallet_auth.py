"""Local, short-lived wallet authentication. No RPC, transactions or private keys."""
import base64
import re
import secrets
import threading
import time
from datetime import datetime, timezone
from http.cookies import SimpleCookie, CookieError

COOKIE = 'jr_wallet_session'
CHALLENGE_TTL = 120
SESSION_TTL = 600
MAX_SESSIONS = 256
_ALPHABET = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def public_key(address):
    # Base58 is only an address codec; Ed25519 verification uses cryptography.
    if not isinstance(address, str) or not 32 <= len(address) <= 44:
        raise ValueError('Endereco Solana invalido')
    number = 0
    for char in address:
        if char not in _ALPHABET:
            raise ValueError('Endereco Solana invalido')
        number = number * 58 + _ALPHABET.index(char)
    raw = bytes(len(address) - len(address.lstrip('1'))) + number.to_bytes((number.bit_length() + 7) // 8, 'big')
    if len(raw) != 32:
        raise ValueError('Endereco Solana invalido')
    return raw


def cookie_token(header):
    try:
        cookies = SimpleCookie()
        cookies.load(header or '')
        token = cookies[COOKIE].value if COOKIE in cookies else ''
        return token if re.fullmatch(r'[A-Za-z0-9_-]{43}', token) else ''
    except CookieError:
        return ''


def _utc(value):
    return datetime.fromtimestamp(value, timezone.utc).isoformat(timespec='seconds')


class AuthStore:
    def __init__(self, clock=time.time):
        self.clock = clock
        self.sessions = {}
        self.lock = threading.Lock()

    def _prune(self):
        now = self.clock()
        for token in list(self.sessions):
            if self.sessions[token]['expires'] <= now:
                del self.sessions[token]

    def challenge(self, token, origin, address):
        key = public_key(address)
        # Fail before opening the wallet if the verifier is unavailable.
        try:
            from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey  # noqa: F401
        except ImportError as exc:
            raise RuntimeError('Instale requirements.txt para autenticar a carteira') from exc
        with self.lock:
            self._prune()
            session = self.sessions.get(token)
            if session is None or session['origin'] != origin:
                if len(self.sessions) >= MAX_SESSIONS:
                    raise RuntimeError('Limite de sessoes atingido; tente depois')
                token = secrets.token_urlsafe(32)
            now = self.clock()
            nonce = secrets.token_urlsafe(32)
            expires = now + CHALLENGE_TTL
            message = ('JrBot - autenticar carteira\n'
                       f'Origem: {origin}\nCarteira: {address}\nAmbiente: solana:devnet\n'
                       f'Nonce: {nonce}\nEmitido em: {_utc(now)}\nExpira em: {_utc(expires)}\n'
                       'Esta assinatura autentica uma sessao local do painel JrBot.\n'
                       'Nao autoriza compra, transferencia de SOL ou uso de Skill licenciada.')
            self.sessions[token] = dict(origin=origin, expires=now + SESSION_TTL, address='',
                challenge=dict(id=nonce, key=key, address=address, message=message, expires=expires))
            return token, dict(id=nonce, message=message, expires_at=expires)

    def verify(self, token, origin, challenge_id, signature):
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
        with self.lock:
            self._prune()
            session = self.sessions.get(token)
            if not session or session['origin'] != origin:
                raise ValueError('Sessao ausente ou expirada; autentique novamente')
            challenge = session.pop('challenge', None)  # One attempt, including invalid signatures.
            session['address'] = ''
            if not challenge or challenge['id'] != challenge_id or challenge['expires'] <= self.clock():
                raise ValueError('Desafio invalido, usado ou expirado')
            try:
                decoded = base64.b64decode(signature, validate=True)
                if len(decoded) != 64:
                    raise ValueError('Assinatura invalida')
                Ed25519PublicKey.from_public_bytes(challenge['key']).verify(decoded, challenge['message'].encode('utf-8'))
            except (InvalidSignature, ValueError, TypeError) as exc:
                raise ValueError('Assinatura invalida') from exc
            session['address'] = challenge['address']
            session['expires'] = self.clock() + SESSION_TTL
            return dict(authenticated=True, address=session['address'], expires_at=session['expires'])

    def status(self, token, origin):
        with self.lock:
            self._prune()
            session = self.sessions.get(token)
            if not session or session['origin'] != origin or not session['address']:
                return dict(authenticated=False)
            return dict(authenticated=True, address=session['address'], expires_at=session['expires'])

    def logout(self, token):
        with self.lock:
            self.sessions.pop(token, None)


STORE = AuthStore()
