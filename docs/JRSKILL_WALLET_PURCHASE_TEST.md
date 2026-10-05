# Etapa 4D — compra pelo navegador

05/10/2026, exclusivamente `V1s-00`. Painel atual **JRBOT-PANEL-V1S-PURCHASE-09** (diagnostico corrigido; implementacao inicial PURCHASE-08). Historico [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33), regras comerciais [#46](https://github.com/JuniorNarciso26/JrRobot/issues/46).

## Escopo e estado

Compra Wallet Standard para a mesma Skill/oferta Devnet validada por A/B. Consulta uma licenca por carteira/Skill e apresenta a Skill de teste quando licenciada. Nao e descoberta geral de todas as Skills da carteira.

**Implementado e testado em software; compra pela extensao Phantom real ainda pendente.** Nenhum novo deploy, compra Devnet ou teste fisico realizado nesta implementacao. Programa, firmware, JSON v1 e Recipe preservados. Executor experimental continua independente da licenca; bloqueio no hardware fica para a etapa seguinte.

## Arquitetura

1. Conectar e autenticar por mensagem, nas Etapas 4A/4B.
2. Consultar minha licenca: backend confirma genesis Devnet e contas finalized; valida owner, discriminator, layout, comprador/Skill/oferta/modelo/parcelas. Carteira vem da sessao autenticada, nunca de parametro livre do browser.
3. Consultar custo: conferir oferta/configuracao e Skill congelada via RPC; preparar transacao unsigned com uma instrucao buy_license. Comprador e unico signer/fee payer. Preco maximo e exatamente o preco exibido. Mostrar recebedores, parcelas, deposito, estimativa da taxa, total e conferir saldo.
4. Marcar aceite e clicar Comprar com a carteira. Wallet Standard solana:signTransaction, conta selecionada, chain solana:devnet, versao legacy. Autenticacao e assinatura de compra sao separadas.
5. Backend verifica assinatura e igualdade exata da mensagem com a cotacao, sessao/origem/carteira, prazo e blockhash; envia ao RPC fixo Devnet com preflight e sem reenvio automatico.
6. Consultar minha licenca novamente: assinatura/RPC que aceitou o envio nao e prova de finalizacao. Somente leitura finalized valida permite mostrar a Skill como licenciada. Link da assinatura abre o Explorer Devnet.

`wallet_purchase.py` usa **solders 0.29.0** para PDAs, compilacao/serializacao da transacao e verificacao da assinatura. Os layouts comerciais pequenos sao desserializados com limites/discriminadores explicitos. O encoding foi comparado com a IDL Anchor gerada: transacoes Python/JS identicas, **417 bytes**.

Referencias: [solders Transaction](https://heavey.dev/solders/api_reference/transaction.html), [Solana Wallet Standard signTransaction](https://github.com/anza-xyz/wallet-standard/blob/master/packages/core/features/src/signTransaction.ts), [solders no PyPI](https://pypi.org/project/solders/0.29.0/).

## Limites de envio

- Rotas `/jrskill/wallet/purchase/`: cookie HttpOnly/SameSite Strict, host/origin local, cabecalho X-JrBot-Panel e autenticacao obrigatoria.
- Quote ligada a token/origin/carteira, TTL 90 segundos, ate 256 entradas em memoria. Nova quote invalida anterior da mesma sessao; nada persistido em disco.
- Rejeitar mensagem/instrucoes alteradas, assinatura invalida/ausente, quote usada/expirada, blockhash expirado, genesis nao Devnet e saldo insuficiente. Sem RPC, destinatarios ou transacoes arbitrarias vindas do browser.
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
2. Reabrir **PAINEL.bat**. Instala requirements se necessario: Python 3.10+, pyserial, cryptography 50.0.1 e solders 0.29.0. Registrar eventual erro pip/porta ocupada. Ctrl+F5, confirmar PURCHASE-09. Sem gravar firmware nem repetir deploy.
3. Na Phantom conferir Testnet Mode/Solana Devnet, conectar carteira de teste, autenticar e Verificar Devnet e saldo. A assinatura pede chain Devnet; suporte Wallet Standard nao revela a rede selecionada na interface da extensao.
4. Consultar minha licenca: usar a Phantom que ainda nao comprou, esperado nao possui. Consultar custo: neste checkpoint 1 SOL, parcelas 0,5/0,5, deposito 0,0013462, taxa estimada 0,000005; conferir os valores atuais apresentados.
5. Conferir comprador/recebedores, marcar aceite, clicar Comprar com a carteira e aprovar na Phantom. Comprador paga preco/deposito/taxa, somente SOL de teste.
6. Consultar minha licenca apos finalizacao: esperado Skill licenciada, License PDA da carteira e assinatura no Explorer. Se pendente, repetir consulta, sem repetir envio.
7. Conferir conta licenciada sem novo pagamento, recusa antes do envio e troca de conta limpando dados. Nao tentar nova compra apos timeout sem consultar a transacao/carteira.
8. Enviar log TXT, assinatura publica e screenshot; nunca seed/chave. Registrar resultado real na #33, separado dos testes simulados.

Proximos itens: descoberta geral das Skills/licencas e autorizacao antes da execucao fisica, com provas de carteira sem/com licenca no hardware.

## Correcao do diagnostico — PURCHASE-09

Log fisico do painel (75) mostra autenticacao, saldo Devnet 5 SOL, cotacoes e licenca ausente. Nao registra assinatura/envio concluido. Consulta RPC independente confirmou licenca ausente, saldo 5 SOL e nenhuma transacao no historico do endereco da licenca. A causa original da falha nao pode ser determinada pelo log antigo: o detalhe HTTP era descartado e erros JSON-RPC eram substituidos por mensagem generica.

A consulta posterior nao diz mais "Compra enviada" quando a tentativa foi incerta. Distingue recebimento pelo RPC de licenca finalized. Tela e TXT registram etapas wallet_signature, signed_transaction_check, submit_http, server_validation, rpc_relay, quote e license_query, com motivo da falha/HTTP status e assinatura publica quando disponivel. Codigo, mensagem e primeiros logs de simulacao RPC sao preservados com limites/redacao; nenhum corpo da transacao, cookie ou chave e registrado. Resultado incerto continua bloqueando reenvio automatico; isso nao confirma pagamento.

Verificacao da correcao: **24 testes Python e 20 JS passaram**, incluindo rejeicao HTTP seguida de consulta sem falsa alegacao de envio, timeout com assinatura/sem retry, logs de validacao do servidor e erro RPC de simulacao sem bytes da requisicao. Bundle recompilado. Nenhuma compra/deploy Devnet ou teste real Phantom realizado nesta correcao. Proximo teste: atualizar checkout Windows, reiniciar painel, conferir PURCHASE-09, conferir Devnet na Phantom, consultar licenca/cotacao e enviar o novo TXT apos uma unica tentativa autorizada. Se houver assinatura pendente, consultar antes de tentar novamente.
