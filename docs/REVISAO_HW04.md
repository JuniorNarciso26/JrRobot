# JrBot - revisao HW04

Correcao da interpretacao anterior e consolidacao dos componentes confirmados pelo proprietario.

Pinagem de audio adotada:
- GPIO21: BCLK/SCK compartilhado.
- GPIO47: WS compartilhado.
- GPIO41: SD do MS3625 para o ESP32.
- GPIO42: DOUT do ESP32 para DIN do MAX98357A.

Camera confirmada: OV5640. OLED: SSD1306 em GPIO1/2, opcional no diagnostico atual.

O MAX98357A possui teste de saida sob demanda. O MS3625 ainda nao possui driver RX nesta revisao.
