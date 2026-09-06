# JrBot - registro da revisao HW03

Solicitacao: nao reutilizar GPIO21/41/42/47, revisar codigo/documentos, organizar materiais/ligacoes e adotar JrBot como identidade unica do produto.

Correcoes: pinagem fixa gerada de manifesto; audio configuravel sem valores automaticos; aprovacao HW03 nova; bloqueio estatico de pinos ocupados, recursos reservados e repeticao de sinais; mapa de camera e OLED preservado; boot permanece sem OLED; painel deixa de sugerir GPIO41 e bloqueia teste de audio com firmware anterior a DIAG-02.

Documentacao: README, contribuicao, plataforma/IA, publicacao, materiais, esquema, pinagem, guias de painel/gravacao, arquivos de instalacao e fontes tecnicas. Referencias obsoletas de outro firmware sao removidas da arvore ativa, sem reescrever historico ou remover licencas de dependencias.

A branch v2-revisada recebe codigo e documentacao. A main recebe somente documentacao publica; nao foi feito merge experimental nem alterada a branch padrao. O nome/URL JrRobot foi preservado. O campo About e separado do README e precisa da acao administrativa indicada em PUBLICACAO.md.

Nao foi confirmado: modelo exato da placa, pinos efetivamente livres para novo audio, funcao dos quatro GPIOs ocupados, modelo/pinagem de microfone, falante e alimentacao final. Esquema consolidado contem pendencias em vez de inventar ligacoes.

Nao houve gravacao na placa, execucao do ESP-IDF do usuario, alteracao de eFuse, dump de credenciais nem exposicao de rede. Testes e limites devem constar no resultado de cada execucao.

Executado nesta revisao: 53 cenarios de compilacao C11 da politica de GPIOs e 10 cenarios JavaScript com DOM minimo simulado, todos passaram. main.c e portal passaram em verificacao de sintaxe GCC com headers simulados; Python e script Bash passaram em checagem sintatica. Nao e build ESP-IDF, execucao Windows, teste fisico nem regressao completa do firmware.
