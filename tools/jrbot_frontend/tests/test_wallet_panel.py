"""Serve the wallet bundle through the exact PAINEL.bat wrapper chain."""
from pathlib import Path
import sys
import threading
import unittest
import urllib.request

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import run_panel_network
import app


class WalletPanelTests(unittest.TestCase):
    def test_served_page_and_local_bundle(self):
        server = app.ThreadingHTTPServer(("127.0.0.1", 0), app.Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        base = f"http://127.0.0.1:{server.server_port}"
        try:
            with urllib.request.urlopen(base) as response:
                page = response.read().decode()
                self.assertIn("JRBOT-PANEL-V1S-WALLET-05", page)
                self.assertIn('src="/wallet_panel.js"', page)
                self.assertIn('id="wallet_provider"', page)
                self.assertIn('id="jrskill_devnet_test"', page)
            with urllib.request.urlopen(base + "/wallet_panel.js") as response:
                self.assertIn("javascript", response.headers["Content-Type"])
                bundle = response.read().decode()
                self.assertIn("wallet-standard:app-ready", bundle)
                self.assertNotIn("signTransaction(", bundle)
        finally:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    unittest.main()
