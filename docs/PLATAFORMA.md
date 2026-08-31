# Plataforma JrBot — direção inicial

## Objetivo

Ter duas camadas desde o começo:

1. **Firmware no ESP32-S3-CAM**: rosto OLED, comandos seriais, status e depois câmera/servo/áudio.
2. **Frontend no computador**: painel simples para instalar/atualizar firmware, mandar comandos e ler respostas/status.

## Primeiro estágio

- Firmware `firmware/face_oled`: testa OLED 128x64 SSD1306/SH1106 por I2C.
- Comandos via Serial/USB: `neutro`, `feliz`, `triste`, `bravo`, `sono`, `esquerda`, `direita`, `surpreso`, `status`.
- Frontend local: roda no computador e envia comandos pela porta serial.

## O que foi aproveitado do backup

- `PetFace_oled`: base direta do desenho dos olhos no OLED e terminal serial.
- `JrRobot` antigo: arquitetura futura de roteamento, estados e contrato de backend/LLM, copiada em `references/` como material de consulta.

## Atualização futura

Depois do primeiro teste físico, evoluir nesta ordem:

1. confirmar pinagem OLED real da ESP32-S3-CAM;
2. adicionar detecção básica de placa/status;
3. adicionar servo da cabeça;
4. adicionar comandos HTTP/MQTT;
5. transformar frontend em painel de atualização, comandos e telemetria;
6. integrar cérebro OpenClaw/VPS somente quando o firmware local estiver estável.
