# Como contribuir com o JrRobot

Obrigado pelo interesse em colaborar com o **JrRobot / Corpo Pablo ESP32**.

Este projeto mistura software, firmware e hardware físico. Por isso, toda contribuição deve priorizar segurança, simplicidade e teste incremental.

## Áreas onde ajuda é bem-vinda

- Expressões para display OLED 128x64.
- Controle de servo da cabeça/pescoço.
- Montagem física simples e leve.
- Alimentação segura com ESP32, servo e módulos.
- Áudio, I2S, TTS ou efeitos sonoros.
- Câmera, visão computacional e acompanhamento humano.
- Documentação para iniciantes.
- Testes em placas ESP32-S3 diferentes.

## Antes de enviar alteração

1. Explique qual problema a mudança resolve.
2. Informe o hardware usado no teste.
3. Não adicione senha, token, API key, chave privada ou Wi-Fi real.
4. Evite mudanças grandes sem abrir uma issue antes.
5. Para hardware, descreva tensão, corrente e pinos usados.

## Segurança de hardware

- Não alimente servo/motor/amplificador pelo pino `3V3` do ESP32.
- Use fonte/regulador adequado para cargas externas.
- Use GND comum quando houver sinal compartilhado.
- Confirme pinagem da sua placa antes de ligar.
- Em dúvida, teste em protoboard antes de soldar.

## Padrão de commits sugerido

Use mensagens curtas e claras, por exemplo:

```text
feat: adiciona expressao feliz no OLED
fix: corrige inicializacao do I2C
docs: melhora pinagem do display
```

## Pull requests

Ao abrir PR, inclua:

- resumo da mudança;
- como testou;
- placa usada;
- fotos/vídeos/logs se for hardware;
- riscos conhecidos ou pendências.
