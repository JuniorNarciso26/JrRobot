#!/usr/bin/env bash
set -euo pipefail
: "${IDF_PATH:?Carregue o ambiente ESP-IDF 5.5.x}"
: "${1:?Informe explicitamente a porta de gravacao, ex. /dev/ttyACM0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python "$ROOT/tools/generate_pinmap.py" --check
cd "$ROOT/firmware"
grep -qx 'JRBOT-V2-DIAG-03' version.txt
python "$IDF_PATH/tools/idf.py" -B build-hw04 -D SDKCONFIG=sdkconfig.hw04 build
grep -qx 'CONFIG_JR_HEADLESS_DIAGNOSTIC=y' sdkconfig.hw04
python "$IDF_PATH/tools/idf.py" -B build-hw04 -D SDKCONFIG=sdkconfig.hw04 -p "$1" flash
printf '%s\n' 'JrBot HW04 gravado. Opere pelo painel na porta de comandos; nenhum monitor foi aberto.'
