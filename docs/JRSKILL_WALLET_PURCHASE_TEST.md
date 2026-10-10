# Etapa 4D — compra pelo navegador

05/10/2026, exclusivamente `V1s-00`. Painel atual **JRBOT-PANEL-V1S-LICENSE-12**; compra ComputeBudget validada na PURCHASE-11. Historico [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33), regras comerciais [#46](https://github.com/JuniorNarciso26/JrRobot/issues/46).

## Escopo e estado

Compra Wallet Standard para a mesma Skill/oferta Devnet validada por A/B. Consulta uma licenca por carteira/Skill e apresenta a Skill de teste quando licenciada. Nao e descoberta geral de todas as Skills da carteira.

**Compra pela extensao Phantom real confirmada em 05/10/2026, log (78), transacao finalized e licenca verificada por RPC independente.** Usuario tambem confirmou visualmente a execucao local e da Skill recuperada da Devnet. Nenhum novo deploy foi necessario. Programa, firmware, JSON v1 e Recipe preservados. Controle de licenca agora implementado no fluxo Devnet do painel, com teste fisico ainda pendente: [Etapa 4E](JRSKILL_LICENSE_EXECUTION_TEST.md). Teste local de desenvolvimento permanece independente.

## Arquitetura

1. Conectar e autenticar por mensagem, nas Etapas 4A/4B.
2. Consultar minha licenca: backend confirma genesis Devnet e contas finalized; valida owner, discriminator, layout, comprador/Skill/oferta/modelo/parcelas. Carteira vem da sessao autenticada, nunca de parametro livre do browser.
3. Consultar custo: conferir oferta/configuracao e Skill congelada via RPC; preparar transacao unsigned com uma instrucao buy_license. Comprador e unico signer/fee payer. Preco maximo e exatamente o preco exibido. Mostrar recebedores, parcelas, deposito, estimativa da taxa, total e conferir saldo.
4. Marcar aceite e clicar Comprar com a carteira. Wallet Standard solana:signTransaction, conta selecionada, chain solana:devnet, versao legacy. Autenticacao e assinatura de compra sao separadas.
5. Backend verifica assinatura e mensagem com a cotacao, sessao/origem/carteira, prazo e blockhash. PURCHASE-11 permite somente o prefixo ComputeBudget limitado descrito abaixo, com a compra e permissoes preservadas. Consulta a taxa da mensagem assinada e saldo antes de enviar ao RPC fixo Devnet com preflight e sem reenvio automatico.
6. Consultar minha licenca novamente: assinatura/RPC que aceitou o envio nao e prova de finalizacao. Somente leitura finalized valida permite mostrar a Skill como licenciada. Link da assinatura abre o Explorer Devnet.

`wallet_purchase.py` usa **solders 0.29.0** para PDAs, compilacao/serializacao da transacao e verificacao da assinatura. Os layouts comerciais pequenos sao desserializados com limites/discriminadores explicitos. O encoding foi comparado com a IDL Anchor gerada: transacoes Python/JS identicas, **417 bytes**.

Referencias: [solders Transaction](https://heavey.dev/solders/api_reference/transaction.html), [Solana Wallet Standard signTransaction](https://github.com/anza-xyz/wallet-standard/blob/master/packages/core/features/src/signTransaction.ts), [solders no PyPI](https://pypi.org/project/solders/0.29.0/).

## Limites de envio

- Rotas `/jrskill/wallet/purchase/`: cookie HttpOnly/SameSite Strict, host/origin local, cabecalho X-JrBot-Panel e autenticacao obrigatoria.
- Quote ligada a token/origin/carteira, TTL 90 segundos, ate 256 entradas em memoria. Nova quote invalida anterior da mesma sessao; nada persistido em disco.
- Rejeitar alteracoes fora do prefixo ComputeBudget autorizado, assinatura invalida/ausente, quote usada/expirada, blockhash expirado, genesis nao Devnet e saldo insuficiente. Sem RPC, destinatarios ou transacoes arbitrarias vindas do browser.
- Carteira ja licenciada nao recebe transacao de compra. Conferir novamente antes do envio; contrato preserva rejeicao de duplicidade adicional.
- Quote consumida antes do relay. Timeout/resultado ambiguo devolve assinatura esperada e estado unknown; nao afirmar sucesso nem reenviar. Cliente impede outro envio enquanto tentativa esta sem confirmacao. Consultar licenca/Explorer antes de outra tentativa. Se HTTP nao devolver assinatura, consultar tambem o historico da carteira.
- Troca de conta/provedor, desconexao ou perda da autenticacao descartam quote, aceite e respostas antigas. Assinatura que termina depois da troca nao e enviada. Desconectar nao cancela transacao que ja saiu para a rede.
- Sem seed/chave/wallet JSON no painel. Bundle servido localmente; usuario nao precisa de Node/npm.

## Verificacoes realizadas

- **23 testes Python passaram**, incluindo 7 novos de compra/HTTP: quote sem envio, assinatura valida, mutacao/unsigned, quote usada/expirada, origem/sessao incorretas, genesis incorreto, saldo insuficiente, blockhash expirado, logout durante RPC, timeout sem retry e identidade/owner.
- **18 testes JS passaram**, incluindo 5 novos de compra: aceite obrigatorio, chain Devnet, expiracao/recusa, assinatura/resposta atrasada apos troca, conta licenciada e resultado ambiguo sem falsa confirmacao/reenvio.
- Bundle compilado com esbuild; teste HTTP do wrapper usado pelo PAINEL.bat confirma PURCHASE-08, campos e bundle servidos.
- Chrome headless: carteira simulada com assinatura Ed25519 real, backend Python real, RPC/relay/licenca simulados. Fluxo completo passou: conectar, autenticar, consultar ausencia, cotar, aceitar, assinar, enviar ao mock e consultar licenca. Tela inspecionada, sem erro JS. **Nao e prova da extensao Phantom real ou compra Devnet.**
- RPC Devnet real pelo novo decoder confirmou a licenca ja adquirida por A; nenhuma escrita. JSON v1 conserva blob ecb748e1bbce58f381e6be0cda68725b2f7ae73d.

## Teste do usuario — Windows/Chrome/Phantom

1. Fechar o servidor antigo. No checkout Windows `C:\Projetos\JrRobot`, conferir branch V1s-00/status limpo; git pull --ff-only origin V1s-00. Atualizar o WSL nao atualiza esse checkout.
2. Reabrir **PAINEL.bat**. Instala requirements se necessario: Python 3.10+, pyserial, cryptography 50.0.1 e solders 0.29.0. Registrar eventual erro pip/porta ocupada. Ctrl+F5, confirmar PURCHASE-11. Sem gravar firmware nem repetir deploy.
3. Na Phantom conferir Testnet Mode/Solana Devnet, conectar carteira de teste, autenticar e Verificar Devnet e saldo. A assinatura pede chain Devnet; suporte Wallet Standard nao revela a rede selecionada na interface da extensao.
4. Consultar minha licenca: usar a Phantom que ainda nao comprou, esperado nao possui. Consultar custo: neste checkpoint 1 SOL, parcelas 0,5/0,5, deposito 0,0013462, taxa sem prioridade 0,000005, teto de rede 0,000105 e total maximo 1,0014512; conferir os valores atuais apresentados antes de aceitar.
5. Conferir comprador/recebedores, marcar aceite, clicar Comprar com a carteira e aprovar na Phantom. Comprador paga preco/deposito/taxa, somente SOL de teste.
6. Consultar minha licenca apos finalizacao: esperado Skill licenciada, License PDA da carteira e assinatura no Explorer. Se pendente, repetir consulta, sem repetir envio.
7. Conferir conta licenciada sem novo pagamento, recusa antes do envio e troca de conta limpando dados. Nao tentar nova compra apos timeout sem consultar a transacao/carteira.
8. Enviar log TXT, assinatura publica e screenshot; nunca seed/chave. Registrar resultado real na #33, separado dos testes simulados.

Proximos itens: descoberta geral das Skills/licencas e autorizacao antes da execucao fisica, com provas de carteira sem/com licenca no hardware.

## Correcao do diagnostico — PURCHASE-09

Log fisico do painel (75) mostra autenticacao, saldo Devnet 5 SOL, cotacoes e licenca ausente. Nao registra assinatura/envio concluido. Consulta RPC independente confirmou licenca ausente, saldo 5 SOL e nenhuma transacao no historico do endereco da licenca. A causa original da falha nao pode ser determinada pelo log antigo: o detalhe HTTP era descartado e erros JSON-RPC eram substituidos por mensagem generica.

A consulta posterior nao diz mais "Compra enviada" quando a tentativa foi incerta. Distingue recebimento pelo RPC de licenca finalized. Tela e TXT registram etapas wallet_signature, signed_transaction_check, submit_http, server_validation, rpc_relay, quote e license_query, com motivo da falha/HTTP status e assinatura publica quando disponivel. Codigo, mensagem e primeiros logs de simulacao RPC sao preservados com limites/redacao; nenhum corpo da transacao, cookie ou chave e registrado. Resultado incerto continua bloqueando reenvio automatico; isso nao confirma pagamento.

Verificacao da correcao: **24 testes Python e 20 JS passaram**, incluindo rejeicao HTTP seguida de consulta sem falsa alegacao de envio, timeout com assinatura/sem retry, logs de validacao do servidor e erro RPC de simulacao sem bytes da requisicao. Bundle recompilado. Nenhuma compra/deploy Devnet ou teste real Phantom realizado nesta correcao. Proximo teste: atualizar checkout Windows, reiniciar painel, conferir PURCHASE-09, conferir Devnet na Phantom, consultar licenca/cotacao e enviar o novo TXT apos uma unica tentativa autorizada. Se houver assinatura pendente, consultar antes de tentar novamente.

## Comparacao detalhada — PURCHASE-10

Log (76): cotacao 11:08:33, assinatura retornou 11:08:41, servidor rejeitou antes de relay. Apenas 8 segundos transcorridos; a mensagem agrupada da PURCHASE-09 ainda nao identifica qual verificacao divergiu. Nenhuma compra concluida e alegada.

PURCHASE-10 separa transaction_decode_failed, signature_invalid, quote_missing (ausente/substituida/consumida), quote_token_mismatch, quote_origin_mismatch, quote_address_mismatch, quote_expired, message_changed, transaction_noncanonical e blockhash_expired. Em message_changed, TXT registra JR_SKILL_PURCHASE_DIAG com idade/prazo restante, campos divergentes, tamanho/hash SHA-256 das mensagens, blockhash, fee payer, header de permissao das contas, chaves publicas, quantidade/programas/contas das instrucoes, tamanho/hash dos dados e primeiros 8 bytes identificadores. Expected e actual sao linhas separadas; ate 8 instrucoes sao descritas. Quote e referenciada por hash curto, sem identificador reutilizavel; nenhum token/cookie, corpo da transacao ou payload completo e registrado. A verificacao estrita da transacao permanece igual.

**25 testes Python passaram**, incluindo instrucao extra assinada e bloqueada com comparacao esperada/recebida, expiracao e cotacao ausente. Nenhuma nova compra/deploy Devnet ou teste Phantom real ocorreu. Proximo teste: reiniciar PURCHASE-10 e enviar TXT contendo JR_SKILL_PURCHASE_DIAG apos a tentativa; diferencas da Phantom real precisam dessa evidencia antes de alterar qualquer regra de assinatura.

## Compatibilidade delimitada — PURCHASE-11

Log (77) identifica a causa: assinatura valida, 80,648 segundos restantes, mesmo blockhash/fee payer e mesma compra (dados e contas), mas duas instrucoes ComputeBudget foram adicionadas antes dela. SetComputeUnitPrice=375000 micro-lamports/CU e SetComputeUnitLimit=200000: prioridade de 75000 lamports (0,000075 SOL). A PURCHASE-10 rejeitou a mudanca antes do relay; nao houve prova de compra concluida.

Politica PURCHASE-11: aceitar mensagem exatamente igual, ou exatamente duas instrucoes ComputeBudget sem contas antes da unica compra, uma de cada tipo (2/3), em qualquer ordem. CU entre 1 e 200000, preco ate 500000 micro-lamports/CU, prioridade ceil(CU*preco/1000000) ate 100000 lamports. Rejeitar duplicadas, outros tipos, tamanhos incorretos, outros programas/instrucoes e contas extras. Preservar blockhash, fee payer, programa/dados/ordem das contas da compra e privilegios globais signer/writable de todas as contas originais. Somente a conta ComputeBudget readonly/nao signer pode ser acrescentada. Serializacao canonica, limite 1232 bytes e verificacao criptografica continuam obrigatorios.

Cotacao armazena e exibe margem de prioridade 0,0001 SOL, teto de rede igual a taxa consultada mais essa margem e total maximo incluindo preco/deposito. Neste checkpoint: teto 0,000105 SOL e total maximo 1,0014512 SOL de teste. Saldo deve cobrir o teto ao cotar. Antes do envio, getFeeForMessage consulta a mensagem efetivamente assinada; se ultrapassar o teto ou houver saldo insuficiente, bloquear. Cotacao e sessao sao verificadas novamente antes de consumir/envio. Nada e removido da transacao assinada. `JR_SKILL_PURCHASE_BUDGET` registra CU/preco/prioridade, taxa consultada e teto; essa taxa consultada nao e alegada como debito final, que depende da transacao finalizada.

Referencia: [Compute Budget](https://solana.com/docs/core/fees/compute-budget), [formula de prioridade](https://solana.com/docs/core/fees/fee-structure).

Verificacao: **29 testes Python e 21 JS passaram**, incluindo parametros observados no log 77, limites/arredondamento, assinatura valida com prefixo, tipos duplicados/desconhecidos, dados/contas de compra alterados, permissao escalada, taxa RPC acima do teto, perda de saldo e aceite com teto consistente. Bundle recompilado. Testes usam carteira/RPC simuladas; nao houve compra/deploy Devnet ou teste real Phantom nesta implementacao. Usuario deve reiniciar PURCHASE-11, conferir Devnet, consultar ausencia/cotacao, aceitar o total maximo e enviar TXT apos uma tentativa, depois consultar licenca finalized.

## Resultado real — Phantom e execucao fisica, log (78)

Teste do usuario em 05/10/2026: 11:44:34 licenca ausente; 11:44:37 cotacao; 11:44:44 retorno da assinatura; 11:44:47 envio; 11:45:18 owned=true. [Transacao Devnet](https://explorer.solana.com/tx/3SSmCuBpQY2qh2EiwY94D3vn1nntxs97pC5kF4qSXpvKmcpKzsVyGfY1bTPN4qBrCJGprzH5CS6Nq8V1wXNRfFP9?cluster=devnet), **finalized slot 507765827**, meta.err=null.

Comprador `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R`, licenca `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`, Skill `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`, modelo 00. RPC independente confirmou dados/identidade da licenca, unica compra correta, prefixo ComputeBudget e repasses. Criador e tesouraria receberam 500000000 lamports cada; deposito da licenca 1346200; taxa efetiva 80000 (75000 prioridade + 5000 base). Debito total **1001426200 lamports = 1,0014262 SOL de teste**, saldo apos transacao 3,9985738 SOL. Taxa ficou abaixo do teto aceito 105000. [Prova RPC publica](HACKATHON_DEVLOG/assets/day6/license-purchase-phantom-proof.json).

Execucao local 11:45:43–11:45:46 result=ok. Leitura Devnet 11:45:48, slot 507766081, 128 bytes, hash verificado e matches_checkpoint=true. Execucao source=solana-devnet 11:45:49–11:45:52 result=ok, sequencia happy -> surprised -> thinking -> happy -> neutral. Usuario confirmou visualmente as faces no hardware. [Trecho selecionado do log (78)](HACKATHON_DEVLOG/assets/day6/panel-log-78-skill-excerpt.txt).

**Compra real e execucao fisica confirmadas; enforcement ainda nao comprovado nem implementado.** Executar depois da compra nao prova que uma carteira sem licenca seja bloqueada: executor experimental permanece independente. Proxima etapa deve consultar/autorizar licenca antes de executar e testar uma carteira sem licenca, uma licenciada, troca/desconexao e indisponibilidade de RPC. Nenhum codigo, firmware ou programa mudou neste registro documental.
