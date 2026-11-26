"""Exercise real C++ capture/transfer code through the Python decoder and WAV writer."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("kws_collect", ROOT / "tools/kws_collect.py")
kws = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kws)
IDENT = "0123456789abcdef"


def frames(pcm):
    yield f"JR_KWS_BEGIN id={IDENT} rate=16000 samples=16000 bytes=32000"
    for sequence, offset in enumerate(range(0, len(pcm), 256)):
        yield f"JR_KWS_DATA id={IDENT} seq={sequence} hex={pcm[offset:offset+256].hex()}"
    yield f"JR_KWS_END id={IDENT} fnv={kws.fnv1a(pcm):08x}"


class CollectorTest(unittest.TestCase):
    def test_wav_exact_pcm_metadata_and_name_validation(self):
        pcm = struct.pack("<16000h", *[i - 8000 for i in range(16000)])
        recording = kws.Recording(IDENT)
        for line in frames(pcm):
            recording.feed(line)
        with tempfile.TemporaryDirectory() as tmp:
            path = kws.save_recording(Path(tmp), "não", "Junior", recording)
            self.assertEqual(path.parent.name, "nao")
            with wave.open(str(path)) as wav:
                self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()),
                                 (1, 2, 16000, 16000))
                self.assertEqual(wav.readframes(16000), pcm)
            metadata = json.loads(path.with_suffix(".json").read_text())
            self.assertEqual(metadata["speaker"], "junior")
            self.assertEqual(metadata["clipped_samples"], 0)
            for invalid in ("../sim", "/tmp/test", "a/b", "a\\b", "", "x" * 33):
                with self.assertRaises(ValueError):
                    kws.safe_name(invalid)

    def test_no_partial_clip_or_corrupt_transfer_can_be_saved(self):
        pcm = bytes(32000)
        stream = list(frames(pcm))
        variants = [stream[:2] + stream[3:],  # lost frame
                    stream[:2] + [stream[1]] + stream[2:],  # duplicate
                    stream[:-1] + [f"JR_KWS_END id={IDENT} fnv=00000000"],
                    [stream[0].replace("rate=16000", "rate=8000")] + stream[1:],
                    stream[:2] + [f"JR_KWS_ERROR id={IDENT} reason=mic_timeout"]]
        for lines in variants:
            recording = kws.Recording(IDENT)
            with self.assertRaises(RuntimeError):
                for line in lines:
                    recording.feed(line)
            with tempfile.TemporaryDirectory() as tmp:
                with self.assertRaises(ValueError):
                    kws.save_recording(Path(tmp), "sim", "pessoa01", recording)
                self.assertEqual(list(Path(tmp).iterdir()), [])
        recording = kws.Recording(IDENT)
        for line in stream[:-1]:
            recording.feed(line)
        self.assertFalse(recording.done)
        self.assertFalse(recording.feed("Detected yes, score: 0.99"))
        self.assertFalse(recording.feed("JR_KWS_END id=other fnv=00000000"))

    @unittest.skipUnless(shutil.which("g++"), "C++ compiler required")
    def test_real_firmware_capture_transfer_roundtrip(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            (folder / "freertos").mkdir()
            # Host doubles supply scheduling/mutex/time, while the actual capture,
            # framing, hashing and serialization are compiled unchanged.
            (folder / "freertos/FreeRTOS.h").write_text('''
#pragma once
#include <mutex>
using portMUX_TYPE = std::mutex;
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(m) (m)->lock()
#define portEXIT_CRITICAL(m) (m)->unlock()
#define pdMS_TO_TICKS(ms) (ms)
#define pdPASS 1
''')
            (folder / "freertos/task.h").write_text('''
#pragma once
#include <thread>
#include <chrono>
inline void vTaskDelay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline int xTaskCreate(void (*)(void*), const char*, int, void*, int, void*) { return 1; }
''')
            (folder / "esp_timer.h").write_text('''
#pragma once
#include <chrono>
#include <cstdint>
inline int64_t esp_timer_get_time() {
 return std::chrono::duration_cast<std::chrono::microseconds>(
 std::chrono::steady_clock::now().time_since_epoch()).count();
}
''')
            (folder / "esp_err.h").write_text('''
#pragma once
#include <cassert>
#define ESP_OK 0
#define ESP_ERR_NO_MEM 1
#define ESP_ERROR_CHECK(e) assert((e) == 0)
''')
            (folder / "main.cc").write_text('''
#include "serial_capture.cc"
#include <cassert>
#include <thread>
int main() {
 assert(!JrCaptureActive());
 std::thread sender([] { record("0123456789abcdef"); });
 while (!JrCaptureActive()) std::this_thread::yield();
 int16_t block[400];
 for (int offset = 0; offset < 16000; offset += 400) {
   for (int j = 0; j < 400; ++j) block[j] = (offset + j) - 8000;
   JrCaptureFeed(block, 400);
 }
 assert(JrCaptureActive());
 sender.join();
 assert(!JrCaptureActive());
}
''')
            exe = folder / "roundtrip"
            subprocess.run(["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=undefined", "-I", str(folder), "-I",
                            str(ROOT / "experiments/tflite-yes-no"), str(folder / "main.cc"),
                            "-o", str(exe)], check=True, capture_output=True)
            output = subprocess.check_output([str(exe)], text=True, timeout=10)
            recording = kws.Recording(IDENT)
            for line in output.splitlines():
                recording.feed(line)
            self.assertTrue(recording.done)
            self.assertEqual(bytes(recording.pcm), struct.pack("<16000h", *range(-8000, 8000)))


if __name__ == "__main__":
    unittest.main()
