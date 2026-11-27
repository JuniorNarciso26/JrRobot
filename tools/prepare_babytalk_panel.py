#!/usr/bin/env python3
"""Prepare pinned BabyTalk for WakeNet -> 5s PCM -> panel log experiment."""
from pathlib import Path
import argparse
import os
import prepare_babytalk_hw04 as source_tools
ROOT = Path(__file__).resolve().parents[1]
def prepare(source, destination=None):
    source_tools.validate(source)
    destination = destination or ROOT / 'firmware/vendor/babytalk'
    original=(source/'components/stt_engine/stt_engine.c').read_text()
    needle='ESP_PARTITION_SUBTYPE_ANY, "model"'
    if original.count(needle)!=1:
        raise RuntimeError('Adaptador de particao incompatível com a fonte fixada')
    destination.mkdir(parents=True,exist_ok=True)
    (destination/'stt_engine.c').write_text(original.replace(needle,'ESP_PARTITION_SUBTYPE_ANY, "stt_model"'),encoding='utf-8')
    relative=os.path.relpath(source,destination).replace('\\','/')
    if '"' in relative or ';' in relative:
        raise RuntimeError('Caminho de fonte incompatível com CMake')
    (destination/'paths.cmake').write_text('set(BABYTALK_ROOT "${CMAKE_CURRENT_LIST_DIR}/'+relative+'")\n',encoding='utf-8')
    print('[OK] BabyTalk verificado; adaptador usa stt_model separado do WakeNet model.')
def configure(config, defaults=None):
    config=Path(config)
    defaults=defaults or ROOT/'firmware/sdkconfig.defaults'
    values={
        'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME':'"partitions_babytalk.csv"',
        'CONFIG_PARTITION_TABLE_FILENAME':'"partitions_babytalk.csv"',
        'CONFIG_ESP32S3_DATA_CACHE_64KB':'y',
        'CONFIG_ESP32S3_DATA_CACHE_32KB':'n',
        'CONFIG_ESP32S3_DATA_CACHE_LINE_64B':'y',
        'CONFIG_ESP32S3_DATA_CACHE_LINE_32B':'n',
        'CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB':'y',
        'CONFIG_ESP32S3_INSTRUCTION_CACHE_16KB':'n',
        'CONFIG_ESP_TASK_WDT_EN':'n',
        'CONFIG_MDNS_MAX_INTERFACES':'2',
        'CONFIG_SR_WN_WN10_PT_OLAESP':'y',
        'CONFIG_SR_MN_EN_NONE':'y',
        'CONFIG_SR_MN_EN_MULTINET6_QUANT':'n',
        'CONFIG_SR_WN_WN9_HIESP':'n',
    }
    lines=(config if config.exists() else defaults).read_text().splitlines()
    for key,value in values.items():
        lines=[line for line in lines if not line.startswith(key+'=') and line!=f'# {key} is not set']
        lines.append(f'# {key} is not set' if value=='n' else f'{key}={value}')
    config.write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path)
    args=parser.parse_args()
    try:
        prepare(args.source.resolve() if args.source else source_tools.ensure_source())
        configure(ROOT/'firmware/sdkconfig.babytalk-panel')
    except (OSError,RuntimeError,source_tools.subprocess.CalledProcessError) as exc:
        parser.exit(1,'[ERRO] '+str(exc)+'\n')
