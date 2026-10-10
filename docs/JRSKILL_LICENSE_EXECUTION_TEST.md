# Etapa 4E — autorizar execucao Devnet por License PDA

05/10/2026, exclusivamente V1s-00. Painel **JRBOT-PANEL-V1S-LICENSE-12**. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**Bloqueio sem licenca, execucao licenciada, interrupcao durante desconexao da carteira e falha de comunicacao, seguida de recuperacao, registrados nos testes do usuario, logs (79) e (80), em 05/10/2026.** RPC independente confirmou os estados das duas carteiras no checkpoint (79). O usuario considerou os testes satisfatorios; este checkpoint nao exige novas simulacoes. Os limites da evidencia estao descritos abaixo. Sem modificacao/deploy do programa, firmware, JSON v1 ou Recipe.

## Arquitetura

- Carteira autenticada habilita a tentativa Devnet. Ao clicar, POST /jrskill/wallet/execution/start valida origem/cookie/sessao, consulta a licenca finalized e carrega a Skill on-chain com hash/schema/autoridade congelados. Sem licenca ou RPC, bloquear antes de comando fisico.
- Backend wallet_execution.py emite permit aleatorio de uma execucao, vinculado a sessao/origem/comprador, TTL 120 segundos, ate 64 entradas. Nova execucao invalida a anterior da mesma sessao. Permit fica em memoria, nao registrado no TXT.
- Sequencia autorizada e derivada pelo servidor da Skill verificada e da Recipe local face_sequence: capabilities, happy, surprised, thinking, happy, neutral. Wait continua no executor local. Endpoint /execution/send aceita somente o proximo comando dessa sequencia, pela Serial, e consulta novamente a licenca finalized antes de cada comando. Sem cache de ownership/fallback local.
- Sessao/comprador/prazo/permit sao conferidos novamente apos RPC. Comando consumido antes do I/O, sem retry automatico; permit removido ao concluir ou falhar. Rejeitar replay, comandos fora de ordem, outra origem/sessao e execucao concorrente.
- Frontend guarda revisao da conta/provedor/autenticacao e confere antes/depois de awaits, waits e comandos. Troca, desconexao ou autenticacao novamente interrompem a execucao anterior. Uma resposta atrasada nao autoriza a nova carteira.
- Log JR_SKILL_AUTHORIZATION: allowed com comprador/licenca, command_sent por comando ou blocked com motivo. Result=ok apenas ao terminar a sequencia sem invalidacao.

## Alcance e limites

Controle do fluxo **Executar Skill da Devnet** no painel/backend. O JSON on-chain permanece publico, e as rotas de leitura continuam publicas. Teste local, controles manuais e /send permanecem ferramentas de desenvolvimento sem licenca; o operador do computador ainda pode controlar seu robo. Nao e DRM no firmware nem protecao contra um cliente modificado.

RPC novo a cada comando pode adicionar latencia; wait=500 nao promete intervalo total de exatamente 500 ms entre faces. Se a execucao for interrompida, comandos ja enviados nao sao desfeitos e a face anterior pode permanecer. Nao enviar uma face neutral automaticamente depois de negar autorizacao. Um comando fisico ja em voo nao pode ser cancelado por desconectar a carteira; os proximos sao bloqueados.

## Verificacao realizada

34 testes Python, 21 JS de carteira e 5 JS do executor passaram. Incluem sequencia completa, ausencia de licenca, autenticacao, expiracao, troca durante RPC, perda de licenca/RPC, substituicao de permit, replay e comando fora de ordem. Teste HTTP com assinatura de autenticacao real de carteira descartavel e RPC/Serial simulados confirma 401 sem sessao, rejeicao sem licenca, 403 origem incorreta, comando autorizado e logout bloqueando o seguinte. Executor simulado confirma nenhuma face em casos negativos e interrupcao apos primeira face quando conta/RPC muda. Bundle recompilado. Nao houve teste fisico desta entrega.

## Plano de teste original — hardware (resultados registrados abaixo)

1. Fechar servidor antigo; atualizar checkout Windows C:\Projetos\JrRobot com git pull --ff-only origin V1s-00; abrir PAINEL.bat, Ctrl+F5, conferir LICENSE-12. Nao gravar firmware nem repetir deploy.
2. Conectar Serial, confirmar HW04/OLED e Verificar Devnet. Sem carteira autenticada, botao Devnet bloqueado; teste local continua funcionando.
3. Conectar/autenticar outra carteira de teste **sem licenca**. Consultar minha licenca deve retornar ausente. Clicar Executar Skill da Devnet: esperado JR_SKILL_AUTHORIZATION result=blocked, sem nenhuma troca de face causada por essa tentativa. Nao comprar com essa carteira para esta prova negativa.
4. Voltar a carteira licenciada 6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R, autenticar/consultar e executar Devnet: esperado allowed, seis command_sent (capabilities e cinco faces), source=solana-devnet/result=ok e faces observadas fisicamente.
5. Desconectar ou trocar carteira durante a sequencia: parar proximos comandos, sem result=ok. Pode permanecer a ultima face ja enviada. Reautenticar para uma nova tentativa.
6. Sem Internet no computador, Devnet deve bloquear; teste local continua disponivel. Restaurar Internet, verificar Devnet e repetir autorizacao. O Wi-Fi do robo nao substitui a conexao RPC do computador.
7. Enviar TXT e observacao visual de cada caso. Registrar resultados fisicos na #33, separados dos testes simulados. Se houver erro, nao alterar parametros sem analisar o motivo no log.

## Resultado — log (79), carteiras sem/com licenca

Carteira nova `8zQwpe3qVBzoPMasLQSbeagtmKzqL1JLG4iEApznUySo` conectada 12:55:43 e autenticada 12:55:49. Consulta 12:55:57: owned=false, PDA derivada `FSmHntJybFcGbhZWLBv4Us4AjSEuG92RYEQAenM1jEMT` ausente. Tentativas 12:56:01, 12:56:12 e 12:56:33 bloqueadas com execution_license_absent. Nenhum command_sent da execucao Devnet aparece para essa carteira. Saldo zero nao e a causa registrada: a autorizacao rejeitou a ausencia de licenca.

Usuario desconectou 12:56:40, voltou a carteira licenciada `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` 12:56:50 e autenticou 12:56:55. Autorizacao allowed 12:56:59, licenca `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`. Skill Devnet slot 507783945, 128 bytes/hash verificado. Seis command_sent (capabilities e cinco faces); sequencia happy -> surprised -> thinking -> happy -> neutral; result=ok source=solana-devnet 12:57:10. Usuario relatou sucesso do teste no robo.

RPC independente confirmou nova carteira owned=false e original owned=true, finalized. [Prova publica dos estados](HACKATHON_DEVLOG/assets/day6/license-gate-wallets-proof.json), [trecho selecionado do log (79)](HACKATHON_DEVLOG/assets/day6/panel-log-79-license-excerpt.txt). A alternancia ocorreu entre tentativas, nao durante uma sequencia ativa; nao alegar validacao fisica desse caso ou de offline nesta prova. Controle e do fluxo Devnet do painel, com controles locais de desenvolvimento preservados. Registro documental sem novo codigo/build/deploy.

## Resultado complementar — interrupcao e recuperacao, log (80)

O usuario realizou testes no robo e considerou os resultados muito satisfatorios. O registro abaixo descreve o que o TXT demonstra, sem exigir repeticao ou novas simulacoes neste checkpoint.

| Horario | Evidencia registrada | Resultado |
| --- | --- | --- |
| 13:04:56 | Carteira licenciada, payload Devnet verificado | Sequencia concluida, result=ok |
| 13:05:01–13:05:02 | Carteira desconectada durante nova sequencia | execution_auth_required; sem conclusao result=ok |
| 13:05:20–13:05:21 | Carteira desconectada durante execucao source=local-file | Teste local concluiu; comportamento previsto do modo de desenvolvimento |
| 13:05:41 | Desconexao durante outra sequencia Devnet | Guarda do frontend interrompeu proximos comandos |
| 13:08:10 | Reautenticacao e nova autorizacao | Execucao Devnet concluida, result=ok |
| 13:08:19 | Evento de conexao com o mesmo endereco durante execucao | Estado da carteira invalidou a sequencia; sem conclusao result=ok |
| 13:09:17 | Carteira sem licenca, consulta owned=false | execution_license_absent; sem envio autorizado de comando para essa carteira |
| 13:09:52 | Retorno a carteira licenciada | Nova sequencia Devnet concluida, result=ok |
| 13:10:05 | Falha de I/O; Remote end closed connection without response | Autorizacao bloqueada; sequencia nao concluida |
| 13:10:25–13:10:37 | Reautenticacao e nova tentativa apos falha | Payload verificado; sequencia concluida, result=ok |

[Trecho selecionado do log (80)](HACKATHON_DEVLOG/assets/day6/panel-log-80-interruption-excerpt.txt). Foram preservadas as linhas de Skill e eventos de estado da carteira; outras linhas foram omitidas.

O TXT evidencia interrupcao por desconexao/invalidation da carteira e falha de comunicacao, seguida de recuperacao. Nao identifica sozinho a causa fisica da falha de I/O (por exemplo, corte de Internet), nem isola troca para um endereco diferente durante a sequencia ativa. Comandos ja enviados nao podem ser desfeitos. A continuidade do teste local sem carteira e intencional; a autorizacao protege o fluxo Devnet do painel, sem DRM no firmware.

Checkpoint aceito pelo usuario com as evidencias recebidas. Esta atualizacao altera apenas documentacao: nenhum novo teste automatizado, build, deploy, pagamento ou alteracao de firmware/programa/JSON/Recipe foi realizado.
