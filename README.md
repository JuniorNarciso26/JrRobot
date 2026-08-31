# jrbot

Projeto principal de trabalho do Corpo Pablo ESP32 / JrBot.

## MVP atual

Rosto OLED para ESP32-S3-CAM com olhos expressivos.

Comandos suportados via Serial/USB:

```text
neutro
feliz
triste
bravo
sono
esquerda
direita
surpreso
status
help
```

## Estrutura

```text
firmware/face_oled/      firmware ESP-IDF do rosto OLED
tools/jrbot_frontend/    frontend local para mandar comandos seriais
scripts/                 scripts de implantação/build/flash
docs/                    documentação de plataforma e pinagem
references/              material útil copiado do backup antigo
```

## Primeiro teste

1. Conectar OLED no ESP32-S3-CAM conforme `docs/PINAGEM.md`.
2. Conectar a placa no computador.
3. Rodar `scripts/deploy_windows.bat` no Windows com ESP-IDF instalado.
4. Atenção nesta placa com duas USB/COM: a gravação pode aparecer em uma porta (ex.: `COM6`) e a Serial de comandos em outra (ex.: `COM4`).
5. Abrir o console serial na porta de comandos e testar `feliz`, `triste`, `bravo` e `status`.
