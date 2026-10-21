import ast
import io
import re
import unittest
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import patch

source = ast.parse(Path('tools/jrbot_frontend/app.py').read_text())
method = next(node for node in source.body if isinstance(node, ast.FunctionDef) and node.name == 'wifi_request')
module = ast.Module(body=[method], type_ignores=[])
env = {'urllib': urllib, 're': re, 'validate_command': lambda command: command,
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

if __name__ == '__main__':
    unittest.main()

