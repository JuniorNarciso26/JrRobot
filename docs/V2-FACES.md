# JrBot Face OLED V2

Versão 2 dos olhos/rosto OLED 128x64.

## Base

A V2 usa o prompt enviado por Junior como referência, mas mantém o firmware em ESP-IDF/C para aproveitar o que já funcionou fisicamente na placa.

## Hardware validado

- ESP32-S3 do Junior
- OLED I2C 128x64 no endereço `0x3C`
- SDA: GPIO 1
- SCL: GPIO 2
- Flash/gravação: COM6 no teste do Junior
- Serial comandos/log: COM4 no teste do Junior

## Expressões implementadas

- `neutro`
- `feliz`
- `triste`
- `animado`
- `bravo`
- `surpreso`
- `pensando`
- `cetico`
- `sono`
- `confuso`
- `piscando`
- `amor`
- `brincalhao`
- `preocupado`
- `cool`
- `bateria`
- `demo`

## Melhorias da V2

- 16 faces no estilo OLED simples.
- Piscar automático em intervalo semi-aleatório.
- Movimento automático leve das pupilas.
- Modo `demo` alternando as expressões.
- Logs `JR_ALIVE`, `JR_STATUS`, `JR_OK` com `v=2`.

## Como testar

Gravar:

```powershell
.\GRAVAR_JRBOT_SEM_MONITOR_WINDOWS.bat COM6
```

Console serial:

```powershell
.\CONSOLE_SERIAL_JRBOT_WINDOWS.bat COM4
```

Depois digitar:

```text
demo
status
feliz
amor
cool
bateria
```


## Painel HTML no navegador

A partir desta revisão, `ABRIR_PAINEL_JRBOT_WINDOWS.bat` abre um painel local em:

```text
http://127.0.0.1:8765
```

Layout:

- lado esquerdo: log serial ao vivo;
- lado direito: botões dos rostos/comandos;
- botões extras: `status`, `help`, `demo on/off` e comando manual.

O painel usa Python local + `pyserial`. Não envia dados para internet.
