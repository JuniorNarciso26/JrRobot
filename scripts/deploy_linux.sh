#!/usr/bin/env bash
set -euo pipefail
: "${IDF_PATH:?Carregue o ambiente ESP-IDF 5.5.x}"
: "${1:?Informe explicitamente a porta de gravacao, ex. /dev/ttyACM0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python "$ROOT/tools/generate_pinmap.py" --check
cd "$ROOT/firmware"
python "$IDF_PATH/tools/idf.py" -B build-hw03 -D SDKCONFIG=sdkconfig.hw03 build
grep -qx 'CONFIG_JR_HEADLESS_DIAGNOSTIC=y' sdkconfig.hw03
python "$IDF_PATH/tools/idf.py" -B build-hw03 -D SDKCONFIG=sdkconfig.hw03 -p "$1" flash
printf '%s\n' 'Gravado. Opere pelo painel na porta de comandos; nao foi aberto monitor.'
