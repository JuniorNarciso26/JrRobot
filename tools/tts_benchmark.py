#!/usr/bin/env python3
"""Repeat JR_TTS_LONG_01 on integrated firmware, via the existing HTTPS Voice API.
No firmware/TTS algorithm changes; compare video acoustics and device metrics.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import ipaddress
import json
from pathlib import Path
import ssl
import time
import urllib.error
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
TEST_ID = "JR_TTS_LONG_01"
PHRASE_SHA256 = "69686839186ec4318b1f0acbc08b50f79d73206f70a29183fd09081a95c7ea82"
SETTINGS = {"enabled": True, "model": "s", "volume": 75, "gain_db": 6,
            "speed": 1.0, "expression": 1.0, "debuzz_db": 0}


def standard_phrase() -> str:
    phrase = (Path(__file__).with_name("tts_benchmark_phrase.txt")).read_text(encoding="utf-8").strip()
    if hashlib.sha256(phrase.encode("utf-8")).hexdigest() != PHRASE_SHA256:
        raise RuntimeError("A frase foi alterada: o hash nao corresponde ao teste isolado JR_TTS_LONG_01.")
    return phrase


def local_origin(url: str) -> str:
    parsed = urllib.parse.urlsplit(url)
    if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password or parsed.query or parsed.fragment or parsed.path not in ("", "/"):
        raise ValueError("Use apenas a origem HTTPS local, exemplo: https://jrbot.local")
    host = parsed.hostname.lower()
    if not host.endswith(".local"):
        try:
            addr = ipaddress.ip_address(host)
        except ValueError as exc:
            raise ValueError("Use jrbot.local ou o IP privado do robo.") from exc
        if not (addr.is_private or addr.is_loopback) or addr.is_unspecified:
            raise ValueError("O destino precisa ser local/privado.")
    return url.rstrip("/")


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


class VoiceClient:
    def __init__(self, origin: str, insecure_local: bool):
        self.origin = local_origin(origin)
        context = ssl._create_unverified_context() if insecure_local else ssl.create_default_context()
        self.opener = urllib.request.build_opener(NoRedirect(), urllib.request.HTTPSHandler(context=context))

    def request(self, payload=None):
        body = None if payload is None else json.dumps(payload, ensure_ascii=False).encode("utf-8")
        headers = {"Cache-Control": "no-store"}
        if body is not None:
            headers.update({"Content-Type": "application/json", "X-JrBot-Voice": "1"})
        req = urllib.request.Request(self.origin + "/voice", data=body, headers=headers)
        # No POST retries: a failed response may follow successful delivery.
        with self.opener.open(req, timeout=15) as response:
            result = json.loads(response.read().decode("utf-8"))
        if not isinstance(result, dict) or result.get("ok") is not True:
            raise RuntimeError("API de voz retornou erro: " + str(result))
        return result


def assert_idle(status):
    if not status.get("available"):
        raise RuntimeError("vTTS nao incluido no firmware instalado.")
    if status.get("busy") or status.get("i2s_owner") != "none":
        raise RuntimeError("Encerre Live, Autonomo e qualquer gravacao/reproducao antes do teste.")


def run(url: str, insecure_local: bool = False):
    phrase = standard_phrase()
    origin = local_origin(url)
    results = ROOT / ".tts-benchmark-results"
    results.mkdir(exist_ok=True)
    path = results / f"{TEST_ID}_develop_{datetime.now(timezone.utc):%Y%m%dT%H%M%S%fZ}.txt"
    with path.open("w", encoding="utf-8", buffering=1) as log:
        def record(message):
            print(message, flush=True)
            log.write(message + "\n")
        record(f"TEST_ID={TEST_ID} stage=integrated transport=https target={origin}")
        record(f"PHRASE_SHA256={PHRASE_SHA256} utf8_bytes={len(phrase.encode('utf-8'))}")
        record("PHRASE=" + phrase)
        record("SETTINGS=" + json.dumps(SETTINGS, sort_keys=True))
        record(f"TLS_CERT_VERIFICATION={'off_local_test_only' if insecure_local else 'on'}")
        record("TIMING=video_enter_to_acoustic_onset_and_end; firmware_first_audio_is_i2s_write")
        sent = False
        try:
            client = VoiceClient(origin, insecure_local)
            assert_idle(client.request())
            client.request({"action": "configure", **SETTINGS})
            status = client.request()
            assert_idle(status)
            if any(status.get(k) != value for k, value in SETTINGS.items()):
                raise RuntimeError("Firmware nao confirmou os mesmos ajustes do teste isolado.")
            completed_before = status["utterances"]
            record("READY settings_confirmed=1 generation_warmup=0")
            print("\nPRONTO. Filme a tela, o teclado e o robo.\n"
                  "Pressione ENTER uma unica vez para enviar a mesma frase do teste isolado.", flush=True)
            input()
            entered = datetime.now(timezone.utc).isoformat()
            start = time.perf_counter()
            print(f"JR_TTS_BENCH_BEGIN {TEST_ID} -- FRASE ENVIADA", flush=True)
            sent = True
            result = client.request({"action": "speak", "text": phrase})
            if result.get("queued") is not True:
                raise RuntimeError("API nao confirmou a fila; nao repetir automaticamente.")
            record(f"HOST_ENTER_UTC={entered} start=before_https_post")
            record(f"HOST_ACK elapsed_ms={(time.perf_counter() - start) * 1000:.1f}")
            deadline = time.monotonic() + 600
            previous_first = 0
            while time.monotonic() < deadline:
                status = client.request()
                if status.get("last_first_audio_ms", 0) and not previous_first:
                    previous_first = status["last_first_audio_ms"]
                    record(f"DEVICE_FIRST_I2S latency_ms={previous_first}; not_acoustic_onset")
                if not status.get("busy"):
                    record("DEVICE_FINAL=" + json.dumps(status, sort_keys=True))
                    if status.get("last_error") != "ESP_OK" or status.get("utterances", 0) != completed_before + 1:
                        raise RuntimeError("Fala nao concluiu com sucesso exclusivo. Consulte DEVICE_FINAL e o log Serial.")
                    record(f"JR_TTS_BENCH_DONE host_observed_ms={(time.perf_counter() - start) * 1000:.1f}")
                    record("RESULT=command_completed; acoustic_onset_continuity_and_end_require_video")
                    break
                time.sleep(0.5)
            else:
                raise RuntimeError("Tempo esgotado. Nenhum reenvio sera feito.")
        except (Exception, KeyboardInterrupt) as exc:
            record(f"TEST_ABORTED {type(exc).__name__}: {exc}")
            if sent:
                record("DELIVERY_MAY_HAVE_OCCURRED=1 do_not_resend_automatically")
            raise
    print(f"\nEnvie o video e este log: {path}")
    return path


def main():
    parser = argparse.ArgumentParser(description="JR_TTS_LONG_01 no JrBot integrado, por HTTPS local")
    parser.add_argument("url", nargs="?", default="https://jrbot.local")
    parser.add_argument("--insecure-local", action="store_true", help="aceitar certificado autoassinado apenas neste teste local")
    args = parser.parse_args()
    try:
        run(args.url, args.insecure_local)
    except (OSError, ValueError, RuntimeError, urllib.error.URLError) as exc:
        parser.exit(1, f"[ERRO] {exc}\nLog preservado. Nao reenvie se o robo ja estiver falando.\n")


if __name__ == "__main__":
    main()
