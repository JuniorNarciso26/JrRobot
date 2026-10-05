# Painel de desenvolvimento — LAYOUT-13

05/10/2026, exclusivamente V1s-00. Identificacao: **JRBOT-PANEL-V1S-LAYOUT-13**. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Organizacao e escopo

O marketplace web sera o ambiente para descobrir/comprar Skills. O app do robo sera o ambiente para selecionar/executar Skills licenciadas. O painel atual continua uma ferramenta de desenvolvimento. Esta entrega reorganiza as funcoes existentes, antes de implementar descoberta geral e selecao de Skills.

- **Robo:** conexao Serial/Wi-Fi, estado do hardware, voz/autonomia, faces, audio, camera, microfone e configuracao Wi-Fi.
- **Skills:** carteira, consulta da licenca da Skill de teste e execucao Devnet. Rede/ajuda ficam recolhidas. Compra e teste local ficam em blocos separados, fechados inicialmente.
- **Diagnostico:** log completo, limpar e baixar TXT. Eventos continuam sendo coletados durante uso das outras abas.

A compra exibe resumo da cotacao com preco, repasses, deposito da licenca, teto da taxa de rede incluindo prioridade e total maximo autorizado. Enderecos e detalhamento completo continuam acessiveis. Aceite explicito, assinatura, validade, verificacao da transacao/licenca e tratamento de envio incerto permanecem os mesmos. Nao ha compra automatica.

IDs dos controles, rotas, autenticacao e autorizacao antes de cada comando foram preservados. Trocar aba apenas muda a visibilidade: nao reconecta carteira/robo, nao reinicia execucao e nao apaga log. Fluxo local continua uma ferramenta sem licenca. Sem alteracao de firmware, contrato, JSON v1 ou Recipe, sem deploy.

## Verificacao desta entrega

34 testes Python, 21 JS de carteira e 5 JS do executor passaram. Bundle de carteira recompilado. Navegador local confirmou versao, navegacao Robo/Skills/Diagnostico, controles bloqueados sem carteira, compra expansivel e log disponivel na aba Diagnostico. Sem extensao Phantom neste navegador de verificacao: nenhuma compra/assinatura nem teste fisico realizados nesta entrega. Os testes fisicos anteriores continuam documentados como LICENSE-12.

## Teste do usuario

1. Fechar o servidor do painel. No checkout Windows usado por PAINEL.bat, confirmar V1s-00 e atualizar com `git pull --ff-only origin V1s-00`. Preservar eventuais mudancas locais antes de atualizar.
2. Abrir PAINEL.bat e recarregar com Ctrl+F5. Conferir **JRBOT-PANEL-V1S-LAYOUT-13**. Nao precisa gravar firmware ou repetir deploy.
3. Em **Robo**, conectar Serial e consultar estado.
4. Em **Skills**, conectar/autenticar a carteira licenciada e clicar **Consultar minha licenca**. Executar a Skill da Devnet e observar as faces.
5. Abrir **Teste de compra — Devnet** para conferir a organizacao. Para consultar uma cotacao nova, usar carteira sem licenca; a carteira licenciada continua protegida contra compra duplicada. Nao e necessario comprar novamente para testar o layout.
6. Em **Diagnostico**, conferir eventos e baixar TXT se houver erro. Voltar a Skills deve preservar estado da carteira/licenca.

Proxima etapa: descoberta e selecao de Skills licenciadas no painel; depois transportar esse fluxo para o app do robo. O marketplace sera desenvolvido como plataforma propria.
