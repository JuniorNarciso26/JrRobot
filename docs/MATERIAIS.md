# JrBot - materiais confirmados

| Qtd. | Item | Identificacao |
|---:|---|---|
| 1 | Placa principal | ESP32-S3-N16R8, duas USB-C, conector de camera |
| 1 | Camera | OV5640 |
| 1 | Display | OLED SSD1306 I2C, 0,96 pol, 128x64, 4 pinos |
| 1 | Microfone | MS3625 MEMS, interface I2S |
| 1 | Amplificador | MAX98357A I2S mono, classe D |
| 1 | Alto-falante | 20 mm / 1 pol, 4 ohms, 3 W |
| 2 | Cabos USB de dados | Um para gravacao e outro para painel nesta montagem |
| conforme necessario | Fios/conectores | Para sinais e alimentacao |

## Pinagem de audio adotada no HW04

- GPIO21: BCLK/SCK compartilhado.
- GPIO47: WS compartilhado.
- GPIO41: SD do microfone -> ESP32.
- GPIO42: ESP32 -> DIN do amplificador.

O driver do microfone ainda nao esta implementado. O teste do amplificador continua sob demanda e nao deve iniciar no boot.
