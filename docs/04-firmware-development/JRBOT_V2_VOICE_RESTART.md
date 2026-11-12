# JrBot V2 — retomada da voz (2026-11-12)

## Base de trabalho
- Base oficial: `main` / JrBot_V1.8.3S, preservada.
- Desenvolvimento: `feature/jrbot-v2-voice`, criada a partir da `main` atual.
- A branch histórica `v2` **não deve ser mesclada integralmente**: está divergente e contém recursos experimentais que precisam ser portados seletivamente.
- Referências: issues #15 e #21; PR histórico #21.

## Primeiro passo efetuado
- Restrito o reconhecimento do nome aos aliases `JR BOT` e `J R BOT`. O alias legado `JUNIOR BOT` foi retirado do novo branch.
- Não houve flash, build local nem validação acústica: código inicial apenas.

## Etapas de implementação
1. Portar para a base atual, seletivamente, os recursos da branch `v2`: calibração sem ação, limiar configurável em runtime, contadores de rejeição e latência. Verificar contratos dos callbacks `jr_voice` e `jr_brain` antes da integração.
2. Validar o ciclo: microfone -> reconhecimento de 'JR BOT' -> resposta 'Oi' -> retorno ao modo escuta. Evitar comandos espúrios; o modo playground não pode disparar ações.
3. Conectar comandos limitados (`JR BOT feliz`, `JR BOT tocar som <asset autorizado>`) às capabilities existentes. Não comandar GPIO/I2S diretamente.
4. Validar gestão de I2S entre `AUTONOMOUS`, `LIVE`, `PLAYBACK` e `IDLE` e manter Wi-Fi/Mesa Mix inalterados.
5. Fazer build com ESP-IDF 5.5.5, testes de software e, depois, validação física de 10 chamadas corretas, testes de falsos positivos, tempos de resposta e RAM/PSRAM.

## Critérios de aceite
- Detecta o nome oficial sem aceitar alias legado.
- Responde e retoma escuta sem travar áudio.
- Não afeta Live, Wi-Fi nem OLED.
- Comandos não reconhecidos não executam ações.
- Registrar medição real; não declarar estabilidade sem teste físico.
