# Aplicar a branch v2-revisada

Revisao atual: **JRBOT-V2-DIAG-03 / JRBOT-HW-04**.

No terminal ESP-IDF 5.5.x:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
DIAG_V2.bat build
DIAG_V2.bat flash
PAINEL.bat
```

COM6 grava e COM4 e usada pelo painel nesta montagem.

O mapa de audio desta revisao e fixo: BCLK=21, WS=47, microfone SD=41 e amplificador DIN=42. O teste do MAX98357A fica desabilitado por padrao ate ser habilitado em `DIAG_V2.bat menuconfig`. O microfone ja tem pinos reservados, mas o driver RX ainda nao foi implementado.
