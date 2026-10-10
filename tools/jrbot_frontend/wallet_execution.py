"""Development-panel license gate for the selected Devnet Skill, not firmware DRM."""
import json
import secrets
import threading
import time
from pathlib import Path
import solana_skill
import wallet_auth
import wallet_purchase

TTL = 120
FACES = ['neutral','happy','sad','excited','angry','surprised','thinking','skeptical','sleepy','confused','winking','love','playful','worried','cool','battery_low']


def commands_for(proof):
    faces = []
    def expand(doc, depth=0):
        if depth > 4 or not isinstance(doc, dict) or type(doc.get('v')) is not int or doc.get('v') != 1 or not isinstance(doc.get('run'), list) or not 1 <= len(doc['run']) <= 64:
            raise ValueError('Skill/Recipe nao suportada pelo autorizador')
        for call in doc['run']:
            if not isinstance(call, list) or len(call) != 2:
                raise ValueError('Chamada invalida')
            fn, value = call
            if fn == 'face' and value in FACES:
                faces.append({'v': 1, 'fn': 'face', 'args': {'expression': value}})
            elif fn == 'wait' and type(value) is int and 0 <= value <= 5000:
                continue
            elif fn == 'recipe' and value == 'face_sequence':
                recipe = Path(__file__).parent / 'jrskill/recipes/face_sequence.json'
                expand(json.loads(recipe.read_text(encoding='utf-8')), depth + 1)
            else:
                raise ValueError('Chamada fora da prova autorizada')
    expand(json.loads(proof['payload_text']))
    return [{'v': 1, 'fn': 'capabilities', 'args': {}}] + faces


class ExecutionStore:
    def __init__(self, auth=None, inspect=None, load=None, clock=time.time):
        self.auth = auth or wallet_auth.STORE
        self.inspect = inspect or wallet_purchase.inspect
        self.load = load or solana_skill.load_skill
        self.clock = clock
        self.lock = threading.Lock()
        self.runs = {}

    def authenticated(self, token, origin, buyer=None):
        status = self.auth.status(token, origin)
        if not status.get('authenticated') or (buyer and status.get('address') != buyer):
            raise ValueError('execution_auth_required: carteira desconectada, alterada ou sessao expirada')
        return status['address']

    def licensed(self, buyer, skill=solana_skill.PDA):
        if skill == solana_skill.PDA:
            proof = self.inspect(buyer)
        else:
            import wallet_skills
            found = wallet_skills.discover(buyer)['skills']
            proof = next((dict(item, owned=True) for item in found if item['skill'] == skill), {'owned': False})
        if not proof['owned']:
            raise ValueError('execution_license_absent: esta carteira nao possui licenca')
        return proof

    def start(self, token, origin, skill=solana_skill.PDA):
        buyer = self.authenticated(token, origin)
        license = self.licensed(buyer, skill)
        proof = self.load() if skill == solana_skill.PDA else self.load(pda=skill)
        if proof.get("pda") != skill:
            raise ValueError("execution_skill_mismatch")
        commands = commands_for(proof)
        self.authenticated(token, origin, buyer)
        permit = secrets.token_urlsafe(24)
        with self.lock:
            self.runs = {key: run for key, run in self.runs.items() if run['expires'] > self.clock() and run['token'] != token}
            if len(self.runs) >= 64:
                raise ValueError('execution_limit: limite de execucoes atingido')
            self.runs[permit] = dict(token=token, origin=origin, buyer=buyer, skill=skill, expires=self.clock()+TTL, commands=commands, index=0, busy=False)
        return dict(proof, execution_permit=permit, buyer=buyer, license=license['license'])

    def send(self, token, origin, permit, command, dispatch):
        with self.lock:
            run = self.runs.get(permit)
            if not run or run['token'] != token or run['origin'] != origin or run['expires'] <= self.clock() or run['busy']:
                raise ValueError('execution_permit_invalid: autorizacao ausente, expirada ou em uso')
            run['busy'] = True
        try:
            self.authenticated(token, origin, run['buyer'])
            parsed = json.loads(command[4:]) if command.startswith('api ') else None
            if not isinstance(parsed, dict) or type(parsed.get('v')) is not int or parsed != run['commands'][run['index']]:
                raise ValueError('execution_command_mismatch: comando fora da sequencia autorizada')
            self.licensed(run['buyer'], run['skill'])  # Fresh finalized RPC read before each physical command.
            self.authenticated(token, origin, run['buyer'])
            with self.lock:
                if self.runs.get(permit) is not run or run['expires'] <= self.clock():
                    raise ValueError('execution_permit_invalid: execucao substituida ou expirada')
                run['index'] += 1  # Consume before I/O; never retry a physical action automatically.
            result = dispatch(command)
            with self.lock:
                if run['index'] == len(run['commands']):
                    self.runs.pop(permit, None)
                run['busy'] = False
            return result
        except Exception:
            with self.lock:
                self.runs.pop(permit, None)
            raise


STORE = ExecutionStore()
