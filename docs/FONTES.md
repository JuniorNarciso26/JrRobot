# JrBot - fontes tecnicas para HW03

Revisao documental em 2026-09-06. O mapa de camera vem do codigo do projeto e nao foi validado contra o esquema da placa. GPIO21/41/42/47 ocupados e OLED em 1/2 foram informados pelo proprietario. Nenhuma fonte generica comprova que um GPIO esta livre nesta PCB.

- Espressif, ESP32-S3 GPIO: pinos existentes, strapping, USB e restricoes de memoria.
  https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/gpio.html
- Espressif, I2C: pull-ups, tempo de resposta e erros.
  https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/peripherals/i2c.html
- Espressif, I2S: API de canais e formatos.
  https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/peripherals/i2s.html
- Analog Devices, MAX98357A: I2S, alimentacao do CI, ausencia de MCLK e limites de saida.
  https://www.analog.com/en/products/max98357a.html
- Adafruit, breakout MAX98357A: saida em ponte, falante entre OUT+ e OUT-, sem conexao ao terra.
  https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts

O exemplo de breakout Adafruit nao identifica o modulo instalado no JrBot. Confirmar o seu modelo antes de aplicar configuracoes de SD/MODE, GAIN ou VIN. Componentes e requisitos podem mudar; usar datasheet e esquema da revisao fisica.
