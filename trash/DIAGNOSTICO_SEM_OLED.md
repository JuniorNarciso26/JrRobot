# JrBot - diagnostico sem OLED

Firmware atual: **JRBOT-V2-DIAG-03**, hardware **JRBOT-HW-04**.

O perfil `headless_diagnostic` nao inicializa o OLED. A falta do display nao deve bloquear painel, Wi-Fi, camera ou testes de audio permitidos.

Pinagem adotada:
- OLED: SDA=GPIO1, SCL=GPIO2, atualmente desabilitado.
- MAX98357A: BCLK=GPIO21, WS=GPIO47, DIN=GPIO42.
- MS3625: SCK=GPIO21, WS=GPIO47, SD=GPIO41.
- Camera: OV5640 no conector da placa.

O microfone ainda nao possui driver de captura. Nao interpretar `pinout_defined` como teste de audio de entrada concluido.

Use `INSTALAR.bat` para compilar, gravar e abrir o painel. Para etapas separadas: `INSTALAR.bat build`, `INSTALAR.bat flash` ou `INSTALAR.bat panel`. Nao e necessario monitor externo.
