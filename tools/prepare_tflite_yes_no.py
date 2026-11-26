#!/usr/bin/env python3
"""Clone a pinned official micro_speech and prepare an isolated HW04 project.

Only the generated copy is patched. The model and feature extractor stay upstream.
No ESP-IDF build, flash, erase or changes to the production firmware happen here.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = "https://github.com/espressif/esp-tflite-micro.git"
REVISION = "3d0b4610fffffbf211d6042824d5d4c408266d93"
WORK = ROOT / ".tflite-yes-no"
COMPONENT_DIR = "espressif__esp-tflite-micro"


def git(path: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(path), *args], text=True).strip()


def validate_source(source: Path) -> None:
    if not (source / ".git").is_dir():
        raise RuntimeError(f"Diretorio sem Git: {source}; nada foi apagado.")
    if git(source, "remote", "get-url", "origin") != UPSTREAM:
        raise RuntimeError("Origem do TensorFlow diferente da oficial; nada foi sobrescrito.")
    if git(source, "rev-parse", "HEAD") != REVISION or git(source, "status", "--porcelain"):
        raise RuntimeError("Checkout TensorFlow alterado ou revisao diferente; nada foi sobrescrito.")


def ensure_source() -> Path:
    source = WORK / COMPONENT_DIR
    WORK.mkdir(exist_ok=True)
    legacy = WORK / "upstream"
    if not source.exists() and legacy.exists():
        # ESP-IDF derives the component identity from the directory basename.
        # Reuse the existing download only after verifying its origin and contents.
        validate_source(legacy)
        legacy.rename(source)
        print(f"[OK] Nome do componente corrigido: {COMPONENT_DIR}", flush=True)
    fresh = not source.exists()
    if fresh:
        subprocess.run(["git", "clone", "--no-checkout", "--depth", "1",
                        UPSTREAM, str(source)], check=True)
    # Never reset edits. A fresh --no-checkout clone has staged deletions until checkout.
    if fresh:
        subprocess.run(["git", "-C", str(source), "fetch", "--depth", "1", "origin", REVISION], check=True)
        subprocess.run(["git", "-C", str(source), "checkout", "--detach", REVISION], check=True)
    validate_source(source)
    return source


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise RuntimeError(f"Patch nao corresponde ao upstream fixado: {path.name}")
    path.write_text(text.replace(old, new), encoding="utf-8")


def prepare(source: Path, project: Path) -> None:
    validate_source(source)
    if source.name not in ("esp-tflite-micro", COMPONENT_DIR):
        raise RuntimeError("A pasta da biblioteca deve se chamar esp-tflite-micro "
                           f"ou {COMPONENT_DIR} para o ESP-IDF resolver o componente.")
    manifest = project / "main" / "idf_component.yml"
    source_path = os.path.relpath(source, manifest.parent).replace("\\", "/")
    manifest_text = ('dependencies:\n  espressif/esp-tflite-micro:\n'
                     '    version: "*"\n    override_path: ' + json.dumps(source_path) + '\n')
    if manifest.exists() and manifest.read_text(encoding="utf-8") != manifest_text:
        # Only generated artifacts of this isolated experiment: cached requirements
        # and the IDF dependency lock can still reference the former /upstream path.
        build = project / "build"
        if build.exists():
            shutil.rmtree(build)
        (project / "dependencies.lock").unlink(missing_ok=True)
        print("[OK] Cache de build/dependencias do teste renovado; sdkconfig preservado.", flush=True)
    # Copy source files only, retaining the isolated build/config/dependency lock on repeat.
    example = source / "examples" / "micro_speech"
    for name in ("main", "test_data"):
        shutil.copytree(example / name, project / name, dirs_exist_ok=True)
    shutil.copy2(example / "partitions.csv", project / "partitions.csv")
    shutil.copy2(source / "LICENSE", project / "LICENSE.upstream")
    (project / "CMakeLists.txt").write_text(
        'cmake_minimum_required(VERSION 3.16)\n'
        'set(IDF_TARGET "esp32s3" CACHE STRING "HW04")\n'
        'if(NOT IDF_TARGET STREQUAL "esp32s3")\n'
        '  message(FATAL_ERROR "Teste HW04 requer esp32s3")\nendif()\n'
        'set(PROJECT_VER "JrBot_V2.2_TEST_YES_NO")\n'
        'set(EXTRA_COMPONENT_DIRS test_data)\n'
        'include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n'
        'project(jrbot_yes_no)\n', encoding="utf-8")
    (project / "sdkconfig.defaults").write_text(
        'CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y\n'
        'CONFIG_COMPILER_OPTIMIZATION_PERF=y\n'
        'CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y\n'
        'CONFIG_PARTITION_TABLE_OFFSET=0xc000\n'
        'CONFIG_PARTITION_TABLE_CUSTOM=y\n'
        'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"\n'
        'CONFIG_ESP_CONSOLE_UART_DEFAULT=y\n', encoding="utf-8")
    # The layout fits in 4 MB even on the larger JrBot module; no erase_flash needed.
    manifest.write_text(manifest_text, encoding="utf-8")
    main = project / "main"
    replace_once(main / "audio_provider.cc", 'static Microphone mic;',
                 'static Microphone mic(GPIO_NUM_21, GPIO_NUM_47, GPIO_NUM_41);')
    replace_once(main / "audio_provider.cc", 'uint8_t g_i2s_read_buffer[i2s_bytes_to_read] = {};',
                 'alignas(4) uint8_t g_i2s_read_buffer[i2s_bytes_to_read] = {};')
    # Match the 32-bit Philips slots already used by JrBot's validated microphone.
    replace_once(main / "i2s_setup.cc", 'slot_cfg.ws_width = I2S_SLOT_BIT_WIDTH_16BIT;',
                 'slot_cfg.ws_width = I2S_SLOT_BIT_WIDTH_32BIT;')
    for call in ('i2s_new_channel(&chan_cfg, NULL, &rx_handle)',
                 'i2s_channel_init_std_mode(rx_handle, &std_cfg)',
                 'i2s_channel_enable(rx_handle)'):
        replace_once(main / "i2s_setup.cc", call + ';', 'ESP_ERROR_CHECK(' + call + ');')
    replace_once(main / "i2s_setup.h", 'i2s_chan_handle_t rx_handle;',
                 'i2s_chan_handle_t rx_handle = nullptr;')
    # Saturate rather than wrap the original x4 microphone scaling on loud input.
    replace_once(main / "audio_provider.cc",
                 '((int16_t *) g_i2s_read_buffer)[i] = ((int32_t *) g_i2s_read_buffer)[i] >> 14;',
                 'int32_t value = ((int32_t *) g_i2s_read_buffer)[i] >> 14;\n'
                 '                if (value > 32767) value = 32767;\n'
                 '                if (value < -32768) value = -32768;\n'
                 '                ((int16_t *) g_i2s_read_buffer)[i] = (int16_t)value;')
    replace_once(main / "audio_provider.cc",
                 'xTaskCreate(CaptureSamples, "CaptureSamples", 1024 * 4, NULL, 10, NULL);',
                 'if (xTaskCreate(CaptureSamples, "CaptureSamples", 1024 * 4, NULL, 10, NULL) != pdPASS) {\n'
                 '        ESP_LOGE(TAG, "Falha ao criar captura");\n'
                 '        return kTfLiteError;\n    }')
    replace_once(main / "audio_provider.cc", 'while (!g_latest_audio_timestamp) {',
                 'int wait_ticks = 0;\n    while (!g_latest_audio_timestamp) {\n'
                 '        if (++wait_ticks > pdMS_TO_TICKS(5000)) {\n'
                 '            ESP_LOGE(TAG, "Sem audio I2S apos 5 segundos");\n'
                 '            return kTfLiteError;\n        }')
    replace_once(main / "audio_provider.cc", 'bytes_read = bytes_read / 2;',
                 'bytes_read = bytes_read / 2;\n'
                 '            static unsigned chunks = 0;\n'
                 '            if (++chunks % 100 == 0) {\n'
                 '                int peak = 0;\n'
                 '                for (unsigned j = 0; j < bytes_read / 2; ++j) {\n'
                 '                    int value = ((int16_t*)g_i2s_read_buffer)[j];\n'
                 '                    if (value < 0) value = -value;\n'
                 '                    if (value > peak) peak = value;\n'
                 '                }\n'
                 '                ESP_LOGI("JR_MIC", "Audio ativo | peak=%d/32768 | heap=%lu",\n'
                 '                         peak, (unsigned long)esp_get_free_heap_size());\n'
                 '            }')
    replace_once(main / "audio_provider.cc", '*audio_samples_size = kMaxAudioSampleSize;',
                 'if (bytes_read != new_samples_to_get * sizeof(int16_t)) return kTfLiteError;\n'
                 '    *audio_samples_size = kMaxAudioSampleSize;')
    replace_once(main / "feature_provider.cc", 'GetAudioSamples((slice_start_ms > 0 ? slice_start_ms : 0),\n'
                 '                      kFeatureDurationMs, &audio_samples_size,\n'
                 '                      &audio_samples);',
                 'if (GetAudioSamples((slice_start_ms > 0 ? slice_start_ms : 0),\n'
                 '                      kFeatureDurationMs, &audio_samples_size,\n'
                 '                      &audio_samples) != kTfLiteOk) return kTfLiteError;')
    replace_once(main / "main_functions.cc", 'uint8_t tensor_arena[kTensorArenaSize];',
                 'alignas(16) uint8_t tensor_arena[kTensorArenaSize];')
    replace_once(main / "main_functions.cc", 'void loop() {',
                 'void loop() {\n  if (!model_input_buffer || !feature_provider || !recognizer) {\n'
                 '    MicroPrintf("JR_YES_NO_FATAL: inicializacao do modelo falhou");\n'
                 '    abort();\n  }')
    replace_once(main / "main_functions.cc", '#include <algorithm>',
                 '#include <algorithm>\n#include <cstdlib>')
    replace_once(main / "main.cc", '#include "esp_log.h"',
                 '#include "esp_log.h"\n#include "driver/gpio.h"')
    replace_once(main / "main.cc", '    loop();', '    loop();\n    vTaskDelay(1);')
    replace_once(main / "main.cc", 'extern "C" void app_main() {',
                 'extern "C" void app_main() {\n'
                 '  gpio_reset_pin(GPIO_NUM_42);\n'
                 '  gpio_set_direction(GPIO_NUM_42, GPIO_MODE_OUTPUT);\n'
                 '  gpio_set_level(GPIO_NUM_42, 0); // keep amplifier quiet\n'
                 '  gpio_set_pull_mode(GPIO_NUM_41, GPIO_PULLDOWN_ONLY);\n'
                 '  ESP_LOGI("JR_YES_NO", "JrBot_V2.2_TEST_YES_NO | BCLK=21 WS=47 MIC=41 | 16000Hz | diga yes/no");\n')
    replace_once(main / "main.cc",
                 '  xTaskCreate((TaskFunction_t)&tf_main, "tensorflow", 8 * 1024, NULL, 8, NULL);',
                 '  if (xTaskCreate((TaskFunction_t)&tf_main, "tensorflow", 8 * 1024, NULL, 8, NULL) != pdPASS) abort();')
    (project / "UPSTREAM_REVISION.txt").write_text(UPSTREAM + "\n" + REVISION + "\n", encoding="utf-8")
    print(f"[OK] TensorFlow oficial {REVISION[:8]} preparado em {project}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, help="Checkout oficial local (validado pelo SHA e sem alteracoes)")
    args = parser.parse_args()
    try:
        prepare(args.source.resolve() if args.source else ensure_source(), WORK / "project")
    except (RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"[ERRO] TensorFlow yes/no: {exc}\n")
