#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../firmware/face_oled"
PORT="${1:-/dev/ttyUSB0}"
command -v idf.py >/dev/null || { echo "ERRO: idf.py nao encontrado. Carregue o ESP-IDF antes."; exit 1; }
idf.py set-target esp32s3
idf.py build
idf.py -p "$PORT" flash monitor
