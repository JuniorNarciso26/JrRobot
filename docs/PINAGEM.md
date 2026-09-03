# Pinagem provisória — JrBot Face OLED

Primeiro teste OLED:

- OLED VCC → 3V3
- OLED GND → GND
- OLED SDA → GPIO 1
- OLED SCL → GPIO 2

Endereço I2C testado automaticamente:

- `0x3C`
- `0x3D`

Atenção: pinagem funcional confirmada pelo teste físico do Junior no OLED. Antes de soldar definitivo, testar em protoboard/jumpers e manter VCC em 3V3, GND comum, SDA no GPIO 1 e SCL no GPIO 2.
