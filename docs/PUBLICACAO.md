# Checklist para tornar o JrRobot público

Este checklist ajuda a preparar o repositório antes de convidar colaboradores.

## Antes de publicar

- [ ] Confirmar que não existe senha, token, chave ou Wi-Fi real no histórico recente.
- [ ] Confirmar que `credencial/`, `.env`, `*.secret` e arquivos locais estão no `.gitignore`.
- [ ] Escolher uma licença aberta: MIT, Apache-2.0 ou GPL-3.0.
- [ ] Criar uma descrição curta do repositório no GitHub.
- [ ] Adicionar topics no GitHub, por exemplo:
  - `esp32`
  - `esp32-s3`
  - `robotics`
  - `oled-display`
  - `embedded`
  - `iot`
  - `ai-robot`
- [ ] Conferir se o README mostra claramente objetivo, status, instalação e roadmap.
- [ ] Criar issues iniciais para colaboradores.

## Issues iniciais sugeridas

1. `Servo da cabeça: definir pinagem e controle PWM`
2. `Adicionar expressões novas para o OLED`
3. `Documentar alimentação segura ESP32 + servo`
4. `Testar em outras placas ESP32-S3`
5. `Planejar módulo de áudio/I2S`
6. `Estudar integração com câmera/acompanhamento humano`
7. `Avaliar cérebro embarcado inspirado em MimiClaw`

## Descrição curta sugerida para o GitHub

```text
Robô físico simples com ESP32-S3: rosto OLED, servo de cabeça, áudio e integração futura com IA/OpenClaw.
```

## Sobre licença

Se a ideia for facilitar colaboração ampla e uso simples, MIT é uma boa primeira opção.

Se quiser proteger mais explicitamente patentes/uso empresarial, considerar Apache-2.0.

Se quiser obrigar derivações abertas, considerar GPL-3.0.
