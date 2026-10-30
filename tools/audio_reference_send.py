"""Send a 16 kHz PCM16 mono voice WAV to the isolated Arduino HW04 sketch."""
from __future__ import annotations

import argparse
import sys
import wave

import serial


def read_reply(port: serial.Serial, expected: str) -> str:
    line = port.readline().decode("ascii", errors="replace").strip()
    if line != expected and not line.startswith(expected + " "):
        raise RuntimeError(f"expected {expected!r}, received {line!r}")
    return line


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="USB CDC port, for example COM6")
    parser.add_argument("wav", nargs="?", help="PCM16 mono 16 kHz WAV, 10 seconds max")
    parser.add_argument("--tone", action="store_true", help="play only the 8 kHz square reference")
    parser.add_argument("--tone-loud", action="store_true", help="play the same tone with amplitude 5000")
    args = parser.parse_args()
    if sum((args.tone, args.tone_loud, bool(args.wav))) != 1:
        parser.error("provide exactly one WAV file, --tone, or --tone-loud")

    pcm = b""
    samples = 0
    if args.wav:
        with wave.open(args.wav, "rb") as wav:
            if (wav.getcomptype(), wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) != ("NONE", 1, 2, 16000):
                parser.error("WAV must be uncompressed PCM16 mono, 16000 Hz")
            samples = wav.getnframes()
            if not 0 < samples <= 160000:
                parser.error("WAV must contain 1 to 160000 samples (10 seconds max)")
            pcm = wav.readframes(samples)
            if len(pcm) != samples * 2:
                parser.error("WAV data is incomplete")

    with serial.Serial(args.port, 115200, timeout=6, write_timeout=6) as port:
        port.reset_input_buffer()
        if args.tone or args.tone_loud:
            port.write(b"T\n" if args.tone else b"L\n")
            print(read_reply(port, "TONE_DONE"))
            return 0
        port.write(f"V{samples}\n".encode("ascii"))
        read_reply(port, "VOICE_READY")
        for offset in range(0, len(pcm), 512):
            port.write(pcm[offset:offset + 512])
            read_reply(port, "CHUNK_OK")
        result = read_reply(port, "VOICE_DONE")
        print(result)
        return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, serial.SerialException) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
