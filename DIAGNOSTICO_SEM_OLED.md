# JrBot - diagnostico sem OLED, revisao HW03

Firmware: JRBOT-V2-DIAG-02. Perfil: headless_diagnostic. O OLED nao e inicializado; sua ausencia nao bloqueia o painel. GPIO1 e GPIO2 continuam reservados.

Toda operacao e pelo painel, em COM4 nesta montagem. Para preparar/gravar use DIAG_V2.bat build e DIAG_V2.bat flash (COM6). Depois PAINEL.bat. Nao e necessario monitor. Ver APLICAR_BRANCH_V2.md.

## Mudanca obrigatoria na pinagem

GPIO21, GPIO41, GPIO42 e GPIO47 ja estao ocupados. Nenhum deles pode ser usado como nova saida de audio ou servo. O antigo mapa de audio e a antiga aprovacao foram revogados. Esta revisao usa tres campos locais, por padrao -1 (sem atribuicao), e uma aprovacao HW03 nova. Valores bloqueados ou duplicados impedem a compilacao.

A proposta BCLK39/WS40/DOUT14 existe apenas para verificar na placa. Nao e uma ligacao confirmada. Consulte docs/PINAGEM.md para inventario completo e docs/ESQUEMA_LIGACAO.md para todas as redes.

## Sequencia de teste

Desligar todas as fontes antes de manipular cabos. Retirar o OLED suspeito sem modificar os quatro GPIOs ocupados. Gravar DIAG-02 e verificar versao no painel. Conferir cada modulo antes de habilitar: amplificador, depois camera. Microfone precisa de modelo/interface/ligacoes antes do driver. Servo sem implementacao.

Audio nao inicia no boot; a permissao HW03 apenas libera o botao de teste. Saida I2S aceita nao comprova som. Camera nao inicia no boot; testa um quadro e devolve recursos, nao valida foco. Nao usar pinos da camera para audio mesmo quando ela esta desligada.

## Validacao

Testes de politica HW03: `python tests/hardware/test_pin_policy.py`. Sao testes host de compilacao/pinos, nao um build ESP-IDF nem aprovacao eletrica. Os resultados historicos em tests/diagnostic e tests/panel correspondem as revisoes indicadas neles; nao sao automaticamente resultados desta mudanca. Build real e ensaio fisico da revisao nova permanecem necessarios.
