# JrBot - aplicar HW03 / DIAG-02

A operacao e pelo painel. Use somente a branch v2-revisada e mantenha o projeto fora da pasta do ESP-IDF.

1. Feche painel antigo e qualquer programa usando as portas. Confira `git status`; preserve mudancas locais.
2. No terminal ESP-IDF 5.5.x, dentro da sua copia do projeto:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
DIAG_V2.bat build
DIAG_V2.bat flash
PAINEL.bat
```

Execute uma linha por vez e pare em qualquer erro; nao use reset --hard ou clean para descartar seu trabalho. O roteiro usa build-hw03 e sdkconfig.hw03, sem reaproveitar aprovacao antiga de audio. O primeiro build usa configuracao nova. Se ja existir trabalho local nessa configuracao, revise antes de alterar.

Gravacao: COM6. Painel/comandos: COM4, 115200, conforme montagem relatada. Windows pode renumerar. O roteiro aceita `JR_FLASH_PORT` para mudar a porta de gravacao. Nao abrir monitor junto com o painel.

No painel, a placa deve responder com JRBOT-V2-DIAG-02 e headless_diagnostic. OLED segue desativado; nenhum teste comeca no boot. Compilar com sucesso nao confirma funcionamento fisico dos modulos.

Audio: nova configuracao tem BCLK=-1, WS=-1 e DOUT=-1. Leia docs/ESQUEMA_LIGACAO.md antes de preencher os GPIOs e confirmar HW03 em `DIAG_V2.bat menuconfig`. Nao reaproveitar autorizacao antiga. Nao gravar eFuses. Nao apagar a flash inteira apenas para trocar esta versao.
