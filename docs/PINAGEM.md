# Pinagem provisória — JrBot Face OLED

Primeiro teste OLED:

- OLED VCC → 3V3
- OLED GND → GND
- OLED SDA → GPIO 1
- OLED SCL → GPIO 2

Endereço I2C testado automaticamente:

- `0x3C`
- `0x3D`

Atenção: pela foto, esta placa parece uma ESP32-S3 WROOM/N16R8 com câmera e pinos GPIO 1 e 2 expostos do lado direito. Antes de soldar definitivo, testar em protoboard/jumpers.
