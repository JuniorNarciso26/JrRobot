#!/usr/bin/env python3
import sys
import time
import threading
try:
    import serial
except Exception:
    print('ERRO: pyserial nao instalado. Rode: python -m pip install pyserial')
    sys.exit(1)

port = sys.argv[1] if len(sys.argv) > 1 else 'COM6'
baud = 115200
running = True

print(f'Conectando em {port} @ {baud}...')
try:
    ser = serial.Serial(port, baud, timeout=0.1)
except Exception as e:
    print(f'ERRO ao abrir {port}: {e}')
    print('Feche idf.py monitor, Arduino Serial Monitor ou qualquer programa usando essa COM.')
    sys.exit(1)

def reader():
    buffer = b''
    while running:
        try:
            data = ser.read(512)
            if data:
                buffer += data
                while b'\n' in buffer:
                    line, buffer = buffer.split(b'\n', 1)
                    print('\n[ESP32]', line.decode('utf-8', errors='replace').rstrip())
                    print('jrbot> ', end='', flush=True)
        except Exception as e:
            print(f'\n[serial read error] {e}')
            break
        time.sleep(0.02)

print('JrBot Serial Console com log ao vivo')
print('Digite: neutro, feliz, triste, bravo, sono, esquerda, direita, surpreso, status, help')
print('Digite sair para fechar.\n')
threading.Thread(target=reader, daemon=True).start()

while True:
    try:
        cmd = input('jrbot> ').strip()
    except (EOFError, KeyboardInterrupt):
        print('\nfechando')
        break
    if cmd.lower() in ('sair', 'exit', 'quit'):
        break
    if not cmd:
        continue
    try:
        ser.write((cmd + '\n').encode('utf-8'))
        ser.flush()
        print(f'[PC] enviado: {cmd}')
    except Exception as e:
        print(f'[PC] erro ao enviar: {e}')

running = False
time.sleep(0.2)
ser.close()
