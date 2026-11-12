# JrBot V2 — teste minimo "Hi ESP" com microfone HW04

Este **projeto ESP-IDF independente** adapta apenas a captura I2S do microfone MS3625 do JrBot para o mecanismo oficial WakeNet da Espressif. Nao edita o ESP-Skainet: o submodulo `../esp-skainet` preserva o exemplo original integralmente.

## Ligacao (ja existente no JrBot HW04)
| MS3625 | ESP32-S3 |
|---|---|
| SCK / BCLK | GPIO21 |
| WS / LRC | GPIO47 |
| SD / DOUT | GPIO41 |
| VDD | 3V3 |
| GND | GND |
| L/R | GND (canal esquerdo) |

Usa I2S 16 kHz (ou sample rate informado pelo modelo), 32 bits mono/esquerda, convertido para PCM 16 bits como no firmware JrBot. Nao usa Wi-Fi, camera, tela, amplificador nem webserver.

## Instalacao de teste
No diretorio `JrRobot/experiments/voice-playground/jrbot-hi-esp` com **ESP-IDF 5.5.5** ativado:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p PORTA flash monitor
```

Use a porta real da placa (`COMx` no Windows ou `/dev/ttyACMx` no Linux), nao o texto literal `PORTA`. Um `idf.py set-target` ou configuracao anterior pode sobrescrever `sdkconfig`: confira com `idf.py menuconfig` se **ESP Speech Recognition > Select wake words > Hi ESP** esta selecionado. O modelo esperado esta em `sdkconfig.defaults`: `CONFIG_SR_WN_WN9_HIESP=y`. Este exemplo usa **ESP-SR 2.5.1**, mesma versao pinada no firmware atual, para limitar variaveis.

## Resultado esperado no monitor Serial
```text
I (...) jr_wake: MIC_READY SCK=21 WS=47 SD=41 rate=16000 model=...
I (...) jr_wake: LISTENING: fale Hi ESP; MIC_LEVEL indica audio captado
I (...) jr_wake: MIC_LEVEL average=123
W (...) jr_wake: HI_ESP_DETECTED count=1
```

Os valores numericos sao **ilustrativos**. `MIC_LEVEL` indica apenas que chegam amostras de audio (deve aumentar ao falar); **somente `HI_ESP_DETECTED` confirma reconhecimento da wake word**. O teste nao produz som nem modifica o OLED.

## Preservacao
- `main` nao foi modificada.
- O exemplo oficial foi mantido intacto no submodulo.
- A gravacao no ESP32-S3 substitui temporariamente o firmware instalado; use a mesma tabela de particoes do JrBot principal para reduzir riscos de sobrescrita de configuracoes, mas faca backup antes e garanta os meios para reinstalar V1.8.3S.
- Nao executar `erase-flash` neste teste.
- Build, flash e validacao fisica ainda nao executados nesta etapa.
