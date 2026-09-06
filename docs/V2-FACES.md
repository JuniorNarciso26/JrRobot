# JrBot - rosto opcional

O renderer de expressoes permanece no projeto, mas o perfil headless_diagnostic nao inicializa OLED. Sua ausencia nao bloqueia os outros modulos. GPIO1/SDA e GPIO2/SCL permanecem reservados.

O driver atual e SSD1306 128x64 em I2C a 100 kHz, modo de paginas. Nao declarar suporte SH1106 sem driver proprio. O retorno de uma transferencia nao comprova imagem correta. Conferir controlador, alimentacao e pull-ups antes de trocar o display.

Use o painel para consultar estado e acionar expressoes quando o modulo for reabilitado. Ver PINAGEM.md e ESQUEMA_LIGACAO.md. A desativacao do OLED nao autoriza remapear seus pinos.
