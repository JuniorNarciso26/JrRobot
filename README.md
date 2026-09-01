# JrBot

Sistema do robô JrBot / Corpo Pablo ESP32.

Versão atual: 2026-09-01 11:32 UTC

## Arquivos principais

Use só estes arquivos na maioria das vezes:

- `INSTALAR.bat` — compila, grava o ESP32-S3 e abre o painel no final.
- `PAINEL.bat` — abre novamente o painel HTML/local depois que já instalou.
- `DIAGNOSTICO.bat` — gera `erro-build.txt` se a compilação falhar.

## Como instalar

1. Extraia o ZIP em uma pasta simples.
2. Abra pelo terminal **ESP-IDF Command Prompt**.
3. Entre na pasta do projeto.
4. Rode:

```powershell
.\INSTALAR.bat COM6
```

Se a porta de gravação não for COM6, troque pelo COM correto.

## Como abrir só o painel depois

```powershell
.\PAINEL.bat
```

O navegador abre em:

```text
http://127.0.0.1:8765
```

## Comandos do rosto

`neutro`, `feliz`, `triste`, `bravo`, `animado`, `surpreso`, `pensando`, `cetico`, `sono`, `confuso`, `piscando`, `amor`, `brincalhao`, `preocupado`, `cool`, `bateria`, `demo`, `status`, `help`.

## Estrutura técnica

- `firmware/esp32/` — firmware ESP-IDF do ESP32-S3.
- `tools/jrbot_frontend/` — painel local HTML/Python.
- `docs/` — pinagem e documentação.
