"""Exercise the real diagnostic transport against a local TLS server."""
import importlib.util
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import shutil
import socket
import ssl
import subprocess
import tempfile
import threading
import unittest
from unittest.mock import patch
import urllib.error

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('phase_bench', ROOT/'tools/tts_benchmark.py')
bench = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bench)


class HTTPSPhasesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('openssl'): raise unittest.SkipTest('openssl unavailable for local TLS fixture')
        cls.tmp = tempfile.TemporaryDirectory()
        cls.cert = Path(cls.tmp.name)/'cert.pem'
        cls.key = Path(cls.tmp.name)/'key.pem'
        subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1',
                        '-subj', '/CN=localhost', '-keyout', str(cls.key), '-out', str(cls.cert)],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()

    def setUp(self):
        requests = self.requests = []
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self): self.respond(None)
            def do_POST(self):
                payload = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
                self.respond(payload)
            def respond(self, payload):
                requests.append((payload, self.headers.get('Connection')))
                action = payload.get('action') if payload else None
                if action == 'drop':
                    self.connection.shutdown(socket.SHUT_RDWR)
                    self.connection.close()
                    return
                if action == 'redirect':
                    self.send_response(302)
                    self.send_header('Location', '/voice')
                    self.end_headers()
                    return
                data = json.dumps({'ok': True, 'queued': action == 'speak'}).encode()
                self.send_response(200)
                self.send_header('Content-Length', str(len(data)))
                self.end_headers()
                self.wfile.write(data)
            def log_message(self, *args): pass
        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(str(self.cert), str(self.key))
        server.socket = context.wrap_socket(server.socket, server_side=True)
        self.server = server
        self.thread = threading.Thread(target=server.serve_forever, daemon=True)
        self.thread.start()
        self.addCleanup(self.stop)
        self.traces = []
        self.client = bench.VoiceClient(f'https://127.0.0.1:{server.server_port}', True)
        self.client.trace = self.traces.append

    def stop(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def test_real_tls_phases_and_existing_fresh_connection_behavior(self):
        self.client.request()
        self.client.request({'action': 'speak', 'text': 'one submission'})
        self.assertEqual(len(self.requests), 2)
        self.assertEqual([headers for _, headers in self.requests], ['close', 'close'])
        self.assertEqual([m['sequence'] for m in self.traces], [1, 2])
        self.assertEqual(self.traces[-1]['action'], 'speak')
        for m in self.traces:
            self.assertTrue(m['success'])
            for key in ('dns_ms', 'tcp_ms', 'tls_ms', 'http_headers_ms', 'body_ms', 'total_ms'):
                self.assertGreaterEqual(m[key], 0)
            self.assertFalse(m['proxy_tunnel'])
            self.assertGreaterEqual(m['total_ms'] + 0.1, m['connect_ms'])

    def test_lost_post_response_is_not_retried(self):
        with self.assertRaises((OSError, bench.http.client.HTTPException)):
            self.client.request({'action': 'drop'})
        self.assertEqual(len(self.requests), 1)
        self.assertEqual(len(self.traces), 1)
        self.assertFalse(self.traces[0]['success'])

    def test_redirect_not_followed(self):
        with self.assertRaises(urllib.error.HTTPError):
            self.client.request({'action': 'redirect'})
        self.assertEqual(len(self.requests), 1)
        self.assertFalse(self.traces[0]['success'])

    def test_cert_verification_still_enforced_by_default(self):
        client = bench.VoiceClient(self.client.origin, False)
        client.trace = self.traces.append
        with self.assertRaises(urllib.error.URLError): client.request()
        self.assertEqual(self.requests, [])
        self.assertFalse(self.traces[0]['success'])
        self.assertIn('tls_ms', self.traces[0])

    def test_dns_failure_is_timed_without_http_submission(self):
        with patch.object(bench.socket, 'getaddrinfo', side_effect=socket.gaierror('DNS failed')):
            with self.assertRaises(urllib.error.URLError): self.client.request({'action': 'speak'})
        self.assertEqual(self.requests, [])
        self.assertIn('dns_ms', self.traces[0])
        self.assertNotIn('tcp_ms', self.traces[0])
        self.assertFalse(self.traces[0]['success'])


if __name__ == '__main__': unittest.main()
