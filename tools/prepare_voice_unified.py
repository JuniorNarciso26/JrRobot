#!/usr/bin/env python3
"""Prepare combined application using the two already pinned voice engines."""
from pathlib import Path
import argparse
import prepare_babytalk_panel as baby
ROOT=Path(__file__).resolve().parents[1]
def prepare(source=None):
    source=source or baby.source_tools.ensure_source()
    baby.prepare(source)
    config=ROOT/'firmware/sdkconfig.voice-unified'
    baby.configure(config,partition='partitions_voice_unified.csv')
    # A 32 KiB data cache returns 32 KiB of SRAM to the application. Measure
    # the latency tradeoff on hardware; keep 64-byte cache lines for SIMD weights.
    text=config.read_text().replace('CONFIG_ESP32S3_DATA_CACHE_64KB=y','# CONFIG_ESP32S3_DATA_CACHE_64KB is not set').replace('# CONFIG_ESP32S3_DATA_CACHE_32KB is not set','CONFIG_ESP32S3_DATA_CACHE_32KB=y')
    config.write_text(text,encoding='utf-8')
    print('[OK] Unified voice: vTTS S+M, WakeNet, BabyTalk; dynamic SRAM and 16 MiB layout.')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path)
    args=parser.parse_args()
    try: prepare(args.source.resolve() if args.source else None)
    except (OSError,RuntimeError,baby.source_tools.subprocess.CalledProcessError) as exc:
        parser.exit(1,'[ERRO] '+str(exc)+'\n')
