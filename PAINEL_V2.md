# JrBot - operar pelo painel

O painel e a interface de comandos e diagnostico. Nao e necessario monitor externo. Nesta revisao eletrica, o firmware esperado e JRBOT-V2-DIAG-02 e o mapa e HW03.

Feche a janela Python do painel antigo, atualize a branch v2-revisada sem descartar alteracoes e abra PAINEL.bat. Mantenha a janela do servidor Python aberta. Conecte Serial USB / COM4 (ou a porta renumerada). COM6 foi informada como gravacao. Nao usar dois programas na mesma COM.

O painel consulta versao e estado. Atualizar apenas o painel nao grava firmware na placa. Use DIAG_V2.bat build e DIAG_V2.bat flash uma vez para instalar a revisao, depois opere pelos botoes.

## Audio HW03

GPIO21/41/42/47 sao ocupados. A mensagem antiga com DIN no GPIO41 foi removida. Audio precisa de tres GPIOs fisicamente confirmados, selecionados na nova configuracao HW03; por padrao estao em -1. A proposta 39/40/14 so esta no esquema como candidata, nao como instrucao aprovada.

O painel nao libera o teste de audio de firmware anterior ao DIAG-02, mesmo se ele responder audio=ready. Verificar firmware nao testa eletricamente pinos. A nova aprovacao de audio ocorre na compilacao, nao e um interruptor remoto.

## Botoes

Atualizar estado mostra o que o firmware respondeu, nao o que o painel presumiu. Testar audio aplica volume (inicial 10%) e aguarda a conclusao I2S; ouvir o falante ainda e necessario. Testar camera faz captura de um quadro por Serial, retorna metadados e nao mostra imagem/autofoco. Microfone sem driver segue indisponivel. OLED desligado nao impede comunicacao.

Baixar log TXT exporta respostas para diagnostico; revisar dados antes de compartilhar. O painel nao reenvia testes automaticamente apos timeout. Configuracao Wi-Fi e Serial, sem registrar a senha, e exige reiniciar a placa apos salvamento.

A interface preserva os controles existentes. Esquema e materiais: docs/ESQUEMA_LIGACAO.md, docs/MATERIAIS.md. A IA ainda e uma etapa de desenvolvimento, nao um botao funcional desta candidata.
