import ast
import io
import json
import re
import unittest
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import patch

source = ast.parse(Path('tools/jrbot_frontend/app.py').read_text())
methods = [node for node in source.body if isinstance(node, ast.FunctionDef) and node.name in {'wifi_request', 'wifi_memory_detail'}]
module = ast.Module(body=methods, type_ignores=[])
env = {'urllib': urllib, 're': re, 'json': json, 'validate_command': lambda command: command,
       'safe_ip': lambda ip: ip, 'NoRedirect': urllib.request.HTTPRedirectHandler,
       'sanitize_log_line': lambda line: line}
exec(compile(module, 'app.py', 'exec'), env)

class MemoryTransport(unittest.TestCase):
    def request(self, reply, command='memoria'):
        with patch('urllib.request.build_opener') as opener:
            opener.return_value.open.return_value = io.BytesIO(reply.encode())
            result = env['wifi_request']('192.168.0.123', command)
            request = opener.return_value.open.call_args.args[0]
            self.assertEqual(request.method, 'POST')
            self.assertEqual(request.data, command.encode())
            return result

    def test_memory_reply(self):
        self.assertEqual(self.request('JR_MEMORY protocol=1 internal_free=8192'),
                         'JR_MEMORY protocol=1 internal_free=8192')

    def test_unexpected_reply(self):
        with self.assertRaises(RuntimeError):
            self.request('JR_OK wrong_reply')

    def test_tasks_reply(self):
        self.assertEqual(self.request('JR_TASKS {"v":1,"total":0,"rows":[]}', 'tarefas'),
                         'JR_TASKS {"v":1,"total":0,"rows":[]}')

    def test_existing_commands(self):
        self.assertTrue(self.request('JR_STATUS protocol=2 version=test', 'status').startswith('JR_STATUS'))
        self.assertTrue(self.request('JR_HELP protocol=2 status memoria', 'help').startswith('JR_HELP'))
        self.assertEqual(self.request('JR_OK test=ok', 'audio_test'), 'JR_OK test=ok')

    def test_full_and_build_proxy(self):
        for kind, query in [('runtime', 'full=1'), ('build', 'build=1')]:
            with patch('urllib.request.build_opener') as opener:
                opener.return_value.open.return_value = io.BytesIO(b'{"v":1,"ready":true}')
                self.assertEqual(json.loads(env['wifi_memory_detail']('192.168.0.123', kind))['v'], 1)
                request = opener.return_value.open.call_args.args[0]
                self.assertEqual(request.full_url, 'http://192.168.0.123/memory?' + query)

    def test_full_proxy_rejects_unknown_kind(self):
        with self.assertRaises(ValueError):
            env['wifi_memory_detail']('192.168.0.123', 'other')

    def test_full_proxy_rejects_non_json(self):
        with patch('urllib.request.build_opener') as opener:
            opener.return_value.open.return_value = io.BytesIO(b'JR_MEMORY protocol=1')
            with self.assertRaises(RuntimeError):
                env['wifi_memory_detail']('192.168.0.123')

if __name__ == '__main__':
    unittest.main()

