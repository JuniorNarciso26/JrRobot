# JrBot

Sistema do robô JrBot / Corpo Pablo ESP32.

Versão atual: 2026-09-01 11:46 UTC

## Arquivos principais

Use só estes arquivos na maioria das vezes:

- `CONFIGURAR_WIFI.bat` — grava o nome/senha do Wi-Fi no firmware local, sem subir segredo para GitHub.
- `INSTALAR.bat` — compila, grava o ESP32-S3 e abre o painel local no final.
- `PAINEL.bat` — abre novamente o painel local pelo computador depois que já instalou.
- `DIAGNOSTICO.bat` — gera `erro-build.txt` se a compilação falhar.

## Credenciais locais

A pasta `credencial/` guarda `wifi.txt` só na máquina do Junior. Ela não vai para o GitHub e não entra nos próximos ZIPs normais, salvo quando for pedido porque a credencial mudou.

Formato:

```text
NOME_WIFI=nome_da_rede
SENHA_WIFI=senha_da_rede
NOME_DO_DISPOSITIVO=jrbot
```

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

- `firmware/` — firmware ESP-IDF do ESP32-S3.
- `docs/` — pinagem e documentação.

## Arquitetura simples

Estrutura atual do pacote:

- `INSTALAR.bat`: compila e grava o firmware.
- `CONFIGURAR_WIFI.bat`: gera a configuração local do Wi-Fi.
- `PAINEL.bat`: abre o painel local/serial no computador.
- `credencial/wifi.txt`: arquivo local para Junior editar rede/senha/IP.
- `firmware/core/`: Wi-Fi, configuração e comandos.
- `firmware/module_face/`: rosto/OLED e expressões.
- `firmware/module_portal/`: painel/API web.
- `firmware/install/`: observação do instalador.

Para o ZIP de teste, não precisamos enviar `docs`, `scripts` e `tools`. Eles podem ficar só comigo como apoio de desenvolvimento, mas o pacote do Junior pode ir enxuto.

Regra do projeto: alterar só o módulo necessário e preservar o que já funcionou no teste físico.
Nota do pacote enxuto: as pastas `docs`, `scripts`, `tools` e `references` não vão mais no ZIP de teste. Elas ficam só no ambiente de desenvolvimento quando forem necessárias.
