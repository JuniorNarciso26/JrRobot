#!/usr/bin/env python3
"""Prepare pinned BabyTalk C engine and verified int4 model for isolated HW04 test."""
from pathlib import Path
import argparse
import hashlib
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / '.babytalk-hw04'
REPO = 'https://github.com/tlack/babytalk.git'
REVISION = '25c9a73b1d54a48b649b781572ac37fdad761d80'
MODEL = 'citrinet256_int4.mmrt'
MODEL_SHA = 'bdb6be064984b86af7c7ead182b2852420edaae252733264cd76f81bad0f8d47'


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()


def validate(source):
    if git(source, 'remote', 'get-url', 'origin') != REPO:
        raise RuntimeError('Origem BabyTalk diferente da oficial; fonte preservada.')
    if git(source, 'rev-parse', 'HEAD') != REVISION or git(source, 'status', '--porcelain'):
        raise RuntimeError('Checkout BabyTalk alterado ou revisao diferente; nao sobrescrito.')
    model = source / 'models' / MODEL
    if not model.is_file() or hashlib.sha256(model.read_bytes()).hexdigest() != MODEL_SHA:
        raise RuntimeError('Modelo int4 ausente/invalido. Nada sera gravado na placa.')


def ensure_source():
    source = WORK / 'upstream'
    WORK.mkdir(exist_ok=True)
    if not source.exists():
        subprocess.run(['git', 'clone', '--no-checkout', '--depth', '1', REPO, str(source)], check=True)
        subprocess.run(['git', '-C', str(source), 'fetch', '--depth', '1', 'origin', REVISION], check=True)
        subprocess.run(['git', '-C', str(source), 'checkout', '--detach', REVISION], check=True)
    validate(source)
    return source


def prepare(source, project):
    validate(source)
    template = ROOT / 'experiments/babytalk-hw04'
    # Retain generated build/config; only tracked application templates are copied.
    shutil.copytree(template, project, dirs_exist_ok=True)
    upstream = os.path.relpath(source, project).replace('\\', '/')
    if '"' in upstream or ';' in upstream:
        raise RuntimeError('Caminho de fonte incompativel com CMake')
    cmake = project / 'CMakeLists.txt'
    cmake.write_text(cmake.read_text().replace('@UPSTREAM@', upstream), encoding='utf-8')
    (project / 'UPSTREAM.txt').write_text(REPO+'\n'+REVISION+'\n'+MODEL_SHA+'\n', encoding='utf-8')
    print('[OK] BabyTalk fixado:', REVISION[:8], '| modelo int4 verificado:', MODEL_SHA[:12], flush=True)
    print('[OK] Projeto isolado:', project, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, help='Checkout oficial local para reutilizar (validado)')
    args = parser.parse_args()
    try:
        prepare(args.source.resolve() if args.source else ensure_source(), WORK / 'project')
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        parser.exit(1, '[ERRO] BabyTalk: '+str(exc)+'\n')
