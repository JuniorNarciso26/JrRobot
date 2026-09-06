# JrBot V2 - diagnostico sem OLED

Marcador desta entrega: `JRBOT-V2-DIAG-01`.

## O que muda

O modo padrao desta candidata e `headless_diagnostic`: o firmware nao inicializa,
nao desenha e nao faz tentativas de I2C no OLED. Os pinos reservados continuam
SDA=GPIO1 e SCL=GPIO2. Terminal e Wi-Fi nao dependem do display.

O log antigo `Modulo face falhou. Parando para proteger o teste atual.` nao
pertence ao main.c desta entrega. Escolher outra COM nao muda o programa
armazenado no mesmo ESP32. Use o marcador acima para distinguir os binarios.

Sem ACK nao prova tela queimada. Se houver suspeita de dano, desligue TODAS as
fontes (inclusive ambas as USBs) e remova os quatro fios do OLED antes de testar.
Nao manipule a ligacao ou o cabo da camera com a placa alimentada.

## Compilacao e gravacao no Windows

Abra o terminal ESP-IDF 5.5.x. Para evitar repetir o binario antigo, use uma copia
nova fora da instalacao do SDK. Nao apague nem sobrescreva suas pastas anteriores.
Se qualquer comando falhar, pare: nao rode o seguinte ignorando o erro.

```bat
cd /d C:\Projetos
git clone --branch v2-revisada --single-branch https://github.com/JuniorNarciso26/JrRobot.git JrRobot-Diag01
cd JrRobot-Diag01
git log -1 --oneline
DIAG_V2.bat build
```

Se `JrRobot-Diag01` ja existir, nao force o clone por cima. Use um nome novo.
O roteiro usa `firmware/build-diag` e `firmware/sdkconfig.diag`, diferentes dos
artefatos antigos. Nao altera o Git do ESP-IDF e nao faz reset/clean destrutivo.

A compilacao deve terminar com `Project build complete`. Em seguida:

```bat
DIAG_V2.bat flash
DIAG_V2.bat monitor
```

O primeiro comando recompila/verifica e grava pela COM6. O segundo abre a COM4.
Esses numeros foram informados para ESTA montagem; o Windows pode renumera-los.
Feche o painel e outros monitores que estejam usando as portas. Para sair do
monitor, pressione Ctrl e ] juntos.

No boot, procure:

```text
BOOT JRBOT-V2-DIAG-01 profile=headless_diagnostic OLED=disabled SDA=1 SCL=2
OLED disabled by configuration; no OLED I2C access
Diagnostic ready: version, status, audio_test, camera_test, mic_test
```

Depois o monitor pode ficar quieto, aguardando comandos; isso nao e travamento.
Digite cada comando e pressione Enter:

```text
version
status
help
```

Se o marcador nao aparecer, nao investigue a tela nem habilite novos modulos:
ainda e preciso confirmar qual binario foi gravado, em qual placa e de qual pasta.

## Amplificador MAX98357A: teste sob demanda

Esta entrega NAO liga o amplificador no boot. O teste so pode acionar os pinos
quando a opcao de confirmacao fisica estiver habilitada. Antes disso, confirme
modulo MAX98357A, alimentacao, falante e pinos realmente livres na sua placa.
GPIO39/40/41 podem estar ligados a JTAG externo/outros circuitos: nao sao
universalmente livres. Nao grave eFuses para este teste.

| Entrada do amplificador | Saida do ESP32 |
| --- | --- |
| BCLK | GPIO39 |
| LRC / WS | GPIO40 |
| DIN | GPIO41 |

Nao deixe o amplificador ainda conectado a GPIO15/16/17: esses pinos pertencem
a camera no mapa existente. Nao altere a alimentacao no chute. Respeite o modulo.

Saia do monitor e, na raiz do projeto, rode:

```bat
DIAG_V2.bat menuconfig
```

Em `JrBot V2 - hardware`, mantenha `Run without OLED` habilitado e marque
`Allow MAX98357A test after verifying GPIO39/40/41` somente apos a conferencia.
Salve e saia. Depois `DIAG_V2.bat flash` e `DIAG_V2.bat monitor`.

Na COM4:

```text
audio_volume 10
audio_test
status
```

O teste solicita um tom curto. `audio_test=tx_completed audible_check=pending`
comprova somente que o driver aceitou os dados I2S sem erro detectado. Nao
comprova que o amplificador esteja ligado nem que o falante tenha emitido som.
Voce precisa confirmar o som fisicamente. Volume comeca baixo por seguranca.

## Camera: uma captura, sem autofocus

Esta entrega nao inicia camera no boot, nao ativa autofocus e nao transmite
video continuo. `camera_test` faz uma captura QVGA/JPEG e libera os recursos.
O comando e exclusivo da Serial; /capture continua indisponivel nesta rodada.

O mapa veio do projeto e precisa corresponder ao hardware instalado:

| Sinal | GPIO |
| --- | --- |
| XCLK | 15 |
| SCCB SDA / SCL | 4 / 5 |
| D0 / D1 / D2 / D3 | 11 / 9 / 8 / 10 |
| D4 / D5 / D6 / D7 | 12 / 18 / 17 / 16 |
| VSYNC / HREF / PCLK | 6 / 7 / 13 |
| PWDN / RESET | -1: sem GPIO controlado pelo firmware |

Nao troque os fios do conector integrado para seguir uma tabela generica.
So habilite `Allow one-shot camera test after verifying the existing camera
pinout` em `DIAG_V2.bat menuconfig` quando o modelo e o mapa estiverem confirmados.
Recompile/grave e monitore com os mesmos comandos do teste de audio.

Na COM4:

```text
camera_test
status
```

Espere a resposta antes de outro comando. O driver pode aguardar alguns segundos
pelo frame. Resposta positiva informa PID, largura, altura, bytes recebidos e
liberacao. Isso testa transporte/captura, NAO imagem visualmente correta ou foco.
Erros indicam a etapa: PSRAM, inicializacao, sensor, frame ou liberacao. Em falha
de liberacao, novos testes sao bloqueados ate reiniciar, evitando alocar por cima.

## Microfone e servo

Nao ha driver nem pinagem de microfone confirmados nesta entrega. `mic_test`
retorna explicitamente `not_configured`; nao inventa leitura nem executa um teste
ficticio. Informe modelo/foto do modulo, sinais e GPIOs antes da implementacao.
O amplificador e uma saida; testa-lo nao valida uma entrada de microfone.
Servo continua sem acionamento nesta rodada.

## Escopo dos testes executados

`python tests/diagnostic/run.py`: 19 cenarios com APIs simuladas e UBSan em GCC.
Cobrem boot sem OLED, boot com OLED falhando, nao iniciar audio/camera mesmo
habilitados, bloqueio de camera nao autorizada, frame ausente/invalido, devolucao
do frame, liberacao e bloqueio apos falha de liberacao. `results.json` registra
esta execucao. Sao testes de fluxo no computador, nao ensaios eletricos.

Esta entrega ainda precisa de build ESP-IDF real e validacao na placa.
O roteiro .bat foi inspecionado, mas nao executado em Windows nesta revisao.

## Atualizacoes futuras

Na copia usada para esta entrega, com monitor fechado:

```bat
git status
git pull --ff-only origin v2-revisada
DIAG_V2.bat build
```

Pare se houver conflito ou erro. Nao use `git reset --hard` para apagar mudancas
locais sem backup. So grave apos build bem-sucedido e validacao do marcador.

## Referencias tecnicas

- ESP-IDF 5.5.5, sistema de build e PROJECT_VER:
  https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-guides/build-system.html
- I2C, ACK/NACK e pull-ups:
  https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/i2c.html
- Camera 2.1.6: inicializacao, devolucao de frames e deinit:
  https://github.com/espressif/esp32-camera/blob/v2.1.6/driver/esp_camera.c
- MAX98357A:
  https://www.analog.com/en/products/max98357a.html
