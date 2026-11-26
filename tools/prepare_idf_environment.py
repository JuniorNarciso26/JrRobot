#!/usr/bin/env python3
"""Validate an existing ESP-IDF environment; never install tools or invent Git tags.
Official 6.1 idf_tools.py records the major.minor family in venv/idf_version.txt.
An older/EIM environment may omit it. Restore only that missing marker after
checking SDK sources, the running venv and official offline dependency checks.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys


def source_version(root: Path, expected: str) -> str:
    cmake = (root / 'tools/cmake/version.cmake').read_text(encoding='utf-8')
    header = (root / 'components/esp_common/include/esp_idf_version.h').read_text(encoding='utf-8')
    values = []
    for key in ('MAJOR', 'MINOR', 'PATCH'):
        c = re.search(r'(?m)^\s*set\s*\(\s*IDF_VERSION_' + key + r'\s+(\d+)\s*\)', cmake)
        h = re.search(r'(?m)^\s*#define\s+ESP_IDF_VERSION_' + key + r'\s+(\d+)\b', header)
        if not c or not h or int(c[1]) != int(h[1]):
            raise ValueError('Versao do SDK ausente ou inconsistente em version.cmake/esp_idf_version.h.')
        values.append(str(int(c[1])))
    if '.'.join(values[:2]) != expected:
        raise ValueError(f'ESP-IDF {expected}.x necessario; fonte encontrada: {".".join(values)}.')
    # idf_tools.py gives version.txt precedence when choosing the Python family.
    version_file = root / 'version.txt'
    if version_file.exists():
        match = re.match(r'^v(\d+\.\d+)', version_file.read_text(encoding='utf-8'))
        if match and match[1] != expected:
            raise ValueError('version.txt do ESP-IDF diverge dos cabecalhos oficiais.')
    if not (root / 'tools/idf.py').is_file() or not (root / 'tools/idf_tools.py').is_file():
        raise ValueError('Instalacao ESP-IDF incompleta: idf.py/idf_tools.py ausente.')
    return '.'.join(values)


def prepare(root: Path, venv: Path, expected: str) -> None:
    version = source_version(root, expected)
    if Path(sys.prefix).resolve() != venv.resolve() or sys.prefix == sys.base_prefix:
        raise ValueError('Execute este verificador com o Python do ambiente ESP-IDF selecionado.')
    marker = venv / 'idf_version.txt'
    if marker.exists() and marker.read_text(encoding='utf-8').strip() != expected:
        raise ValueError('Ambiente Python pertence a outra versao do ESP-IDF; identificador preservado.')
    env = os.environ.copy()
    env.update(IDF_PATH=str(root), IDF_PYTHON_ENV_PATH=str(venv))
    # Official check uses installed requirements/constraints only (no pip install).
    subprocess.run([sys.executable, str(root / 'tools/idf_tools.py'),
                    'check-python-dependencies'], env=env, check=True)
    if not marker.exists():
        # Exclusive creation prevents overwriting a marker created concurrently.
        try:
            with marker.open('x', encoding='utf-8') as output:
                output.write(expected)
        except FileExistsError:
            if marker.read_text(encoding='utf-8').strip() != expected:
                raise ValueError('Identificador Python alterado durante a verificacao.')
        print('[OK] Identificador Python recuperado:', marker)
    print(f'[OK] ESP-IDF {version} validado pelos arquivos oficiais; dependencias Python verificadas.')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    paths = parser.add_mutually_exclusive_group(required=True)
    paths.add_argument('--export', type=Path)
    paths.add_argument('--idf-root', type=Path)
    parser.add_argument('--python-env', type=Path)
    parser.add_argument('--expected', default='6.1')
    parser.add_argument('--version-only', action='store_true')
    args = parser.parse_args()
    root = (args.export.parent if args.export else args.idf_root).resolve()
    try:
        if args.version_only:
            print('[OK] ESP-IDF ' + source_version(root, args.expected) + ' (fonte oficial).')
        elif args.python_env:
            prepare(root, args.python_env.resolve(), args.expected)
        else:
            raise ValueError('--python-env e obrigatorio para validar/preparar o ambiente.')
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f'[ERRO] Ambiente ESP-IDF: {exc}\nRepare a instalacao pelo EIM se necessario.\n')


if __name__ == '__main__':
    main()
