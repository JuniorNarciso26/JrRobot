# JrBot

Sistema do robô JrBot / Corpo Pablo ESP32.

Versão atual: 2026-09-01 11:46 UTC

## Arquivos principais

Use só estes arquivos na maioria das vezes:

- `CONFIGURAR_WIFI.bat` — grava o nome/senha do Wi-Fi no firmware local, sem subir segredo para GitHub.
- `INSTALAR.bat` — compila, grava o ESP32-S3 e abre o painel local no final.
- `PAINEL.bat` — abre novamente o painel local pelo computador depois que já instalou.
- `DIAGNOSTICO.bat` — gera `erro-build.txt` se a compilação falhar.

## Como instalar com Wi-Fi

1. Extraia o ZIP em uma pasta simples.
2. Abra pelo terminal **ESP-IDF Command Prompt**.
3. Entre na pasta do projeto.
4. Configure o Wi-Fi:

```powershell
.\CONFIGURAR_WIFI.bat
```

5. Grave no ESP32:

```powershell
.\INSTALAR.bat COM6
```

Se a porta de gravação não for COM6, troque pelo COM correto.

Depois de iniciar, o ESP32 mostra no log serial uma linha parecida com:

```text
JR_WIFI conectado ip=192.168.0.xxx
```

Aí abra no navegador:

```text
http://IP_DO_ESP32/
```

Exemplo:

```text
http://192.168.0.50/
```

## Como abrir só o painel local depois

```powershell
.\PAINEL.bat
```

O painel local do computador abre em:

```text
http://127.0.0.1:8765
```

## Comandos do rosto

`neutro`, `feliz`, `triste`, `bravo`, `animado`, `surpreso`, `pensando`, `cetico`, `sono`, `confuso`, `piscando`, `amor`, `brincalhao`, `preocupado`, `cool`, `bateria`, `demo`, `status`, `help`.

## Estrutura técnica

- `firmware/esp32/` — firmware ESP-IDF do ESP32-S3.
- `tools/jrbot_frontend/` — painel local HTML/Python do computador.
- `docs/` — pinagem e documentação.
