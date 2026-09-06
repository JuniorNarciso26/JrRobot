# JrBot - operar pelo painel

O painel e a interface principal. Firmware esperado: **JRBOT-V2-DIAG-03 / HW04**.

- COM4: painel/comandos nesta montagem.
- COM6: gravacao nesta montagem.
- Nao abra monitor externo na mesma COM usada pelo painel.

O painel mostra estado do OLED, amplificador, camera e microfone. O amplificador usa BCLK=21, WS=47 e DIN=42. O microfone MS3625 reserva SCK=21, WS=47 e SD=41, mas o driver de captura ainda nao esta implementado.

O botao de audio so fica disponivel quando o firmware confirma que o teste do MAX98357A foi habilitado na compilacao. A camera OV5640 continua com teste de um quadro por Serial. OLED desligado nao impede os demais controles.
