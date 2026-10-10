# JrSkill License PDA 00 — implementacao e prova local

Data: 02/10/2026. Branch exclusiva: `V1s-00`. Historico comercial: [#46](https://github.com/JuniorNarciso26/JrRobot/issues/46); historico tecnico: [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Resultado e limites

| Verificacao | Resultado |
| --- | --- |
| Implementacao Rust e scripts de configuracao/oferta/compra/leitura | Concluida |
| Anchor 1.1.2: compilacao SBF e geracao de IDL | Passou no WSL |
| `npm test` | 6 testes passaram |
| `npm run test:idl` | 2 testes passaram; codec da Skill original preservado |
| `anchor test --validator legacy ...` | 2 testes de integracao passaram, incluindo a prova comercial abaixo |
| Consulta publica da Skill original na Devnet | Passou, compromisso finalized, slot 506702089 |
| Upgrade comercial na Devnet | Realizado pelo usuario; consulta RPC confirma slot 506708700 |
| Primeira compra na Devnet | Realizada em 05/10/2026; RPC finalized no slot 507724452 |
| Compra via Phantom no painel / bloqueio por licenca no hardware | Nao implementados nesta prova |

Ambiente usado: Ubuntu/WSL, Node 24.10.0, Anchor 1.1.2 e Solana CLI 3.1.10. O validator local usa `[test] upgradeable = true`, pois a inicializacao valida a autoridade de upgrade. Sem essa opcao o programa carregado para teste fica sem autoridade administrativa. Referencia: [configuracao oficial do Anchor](https://www.anchor-lang.com/docs/references/anchor-toml#upgradeable).

Programa: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`. SHA-256 do SBF compilado neste ambiente: `e7499f6e162354a741ce548f7d26edcd57bda22a2113efded63945315825b5aa`. Isso identifica o artefato local; nao e atestado de build reproduzivel nem de upgrade na Devnet.

## Casos locais conferidos

Carteiras descartaveis distintas para administrador, criador, tesouraria, A, B e tentativas invalidas. Preco local: **100000001 lamports**, comissao **5000 bps**. A recebe uma License PDA com o comprador/Skill/oferta/modelo corretos; criador recebe **50000001**, tesouraria **50000000**. Arredondamento preserva a soma exata. A paga preco + deposito da conta de 137 bytes; administrador paga a taxa da transacao de teste.

B comeca sem licenca e, ao comprar, recebe sua propria PDA para a mesma Skill. A tentativa duplicada de A nao altera os saldos dos recebedores. Inicializacao por terceiro, percentual acima de 10000, oferta por terceiro, preco zero, redefinicao da configuracao/oferta, limite de preco insuficiente, criador/tesouraria incorretos e comprador igual a recebedor sao rejeitados.

Dois testes enviam transacoes falhas ao validator com preflight desativado e conferem o registro da falha e os saldos/contas posteriores:

- saldo suficiente para criar a conta e pagar ao criador, mas insuficiente para a comissao: ambos os repasses revertem e a licenca fica ausente;
- compra seguida de uma segunda instrucao que falha: o JrSkill termina com sucesso na primeira instrucao, mas a transacao inteira reverte os repasses e a licenca.

Taxas de rede continuam separadas: falhas executadas na rede podem consumir taxa do fee payer. Testes de inteiros do cliente cobrem piso da comissao, percentuais 0/10000, maior u64 e entradas fora do intervalo. O calculo Rust usa u128; nao se alega pagamento real de um preco u64 maximo.

O teste antigo de criar/ler Skill continua passando. A Skill usada no teste comercial tambem conserva os bytes apos compras/falhas. Consulta independente na Devnet: PDA `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`, payload **128 bytes**, hash `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`. JSON v1 no Git conserva o blob `ecb748e1bbce58f381e6be0cda68725b2f7ae73d`.

## Repetir no VS Code / WSL

Na raiz `/home/user/JrRobot`, confira que `git branch --show-current` informa `V1s-00`. Com o checkout limpo, atualize somente essa branch:

```bash
git pull --ff-only origin V1s-00
cd solana/jrskill-solana
npm ci
npm test
```

Preserve a chave do programa existente; nao gere outra nem execute `anchor init`. Confira seu endereco publico antes de compilar:

```bash
solana-keygen pubkey target/deploy/jrskill-keypair.json
# Deve ser Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454
anchor build --provider.cluster devnet
npm run test:idl
if [ ! -f target/local-test-wallet.json ]; then
  solana-keygen new --silent --no-bip39-passphrase --outfile target/local-test-wallet.json
fi
anchor test --validator legacy --provider.cluster localnet --provider.wallet target/local-test-wallet.json
```

Os testes exigem validator limpo; Anchor inicia o seu. Se ja existir um processo na porta 8899, encerre apenas o validator local conhecido antes de repetir.

## Upgrade Devnet realizado e proxima prova de compra

O usuario compilou, passou os 6 testes cliente, 2 IDL e 2 integracao local e realizou o upgrade do mesmo programa. Assinatura informada: `3Bdw92brd5inoDoMmg7q3SzcMrpDsctoL5pd63r71kMbSM37k6yHCvo6nvpQrF7HBxCXoUoZaF6mmRDPSDWnNYqT`. O CLI ampliou os dados de 191976 para 331392 bytes e atualizou a IDL. Consulta RPC independente confirmou Last Deployed In Slot 506708700, mesma autoridade e mesmo ProgramData. Verificador publico posterior, finalized no slot 506709949, confirmou os 128 bytes/hash originais.

A sequencia abaixo fica como referencia do upgrade ja realizado, nao instrucao para repetir agora. Configuracao, oferta e primeira compra Devnet foram realizadas; evidencias no checkpoint de 05/10 abaixo. Use somente SOL de teste; nao troque chaves nem remova a autoridade de upgrade.

```bash
export ANCHOR_PROVIDER_URL=https://api.devnet.solana.com
export ANCHOR_WALLET="$HOME/.config/solana/id.json"
solana address --keypair "$ANCHOR_WALLET"
# Autoridade original: 3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij
solana program show Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454 --url devnet
npm run verify:public
anchor build --provider.cluster devnet
npm run test:idl
anchor program deploy --provider.cluster devnet --provider.wallet "$ANCHOR_WALLET"
npm run verify:public
```

Interrompa antes do deploy se as chaves/autoridade nao corresponderem ou algum teste falhar. O upgrade atualiza o mesmo programa e a IDL, mantendo a conta Skill. Se a ferramenta indicar que falta espaco na conta do programa, registre a mensagem antes de prosseguir; nao publique em outro Program ID.

Depois do upgrade, definir explicitamente a carteira publica de tesouraria e a comissao da prova. Para conferir os repasses separadamente, escolher tesouraria diferente do criador e do comprador. Inicializar uma unica vez com a autoridade administrativa. A oferta exige a wallet criadora da Skill (a mesma wallet para a Skill deste checkpoint):

```bash
# Substitua ENDERECO_PUBLICO_TESOURARIA antes de executar.
npm run market:devnet -- configure ENDERECO_PUBLICO_TESOURARIA 5000
npm run market:devnet -- offer 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000
```

5000 bps e 1 SOL sao termos explicitos deste exemplo, nao valores aprovados como tabela definitiva. Configuracao e oferta nao tem comando de redefinicao na 00; confira os argumentos antes de enviar.

Para prova isolada de compra via CLI, usar uma **wallet local descartavel de teste** diferente dos recebedores, com saldo Devnet. `ANCHOR_WALLET` aponta para sua chave local; nao exporte a chave/seed da Phantom. Consultar cotacao nao envia compra; `--send` envia explicitamente:

```bash
# ANCHOR_WALLET deve apontar aqui para a wallet COMPRADORA local de teste.
npm run market:devnet -- buy 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000 --quote
npm run market:devnet -- buy 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000 --send
npm run market:devnet -- read ENDERECO_PUBLICO_COMPRADOR 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux
```

A cotacao separa preco, parcelas, deposito e estimativa da taxa. Se ja existe licenca valida, o script somente consulta, sem enviar transacao. A leitura publica nao exige wallet; exige IDL local gerada, deps npm e RPC Devnet. Verifica owner, discriminator, PDA, comprador, Skill, oferta, modelo e parcelas registradas.

A carteira Phantom com 5 SOL de teste permanece pronta para a etapa seguinte: compra assinada pela extensao no painel. Esta entrega nao envia transacao por ela. Registrar na #46 assinatura, carteiras, licenca e valores quando a prova Devnet ocorrer; os testes locais nao substituem essa evidencia.

## Primeira compra Devnet — 05/10/2026

Configuracao: `6k8BdUgobWSuwjUTiTvGgBBtK8Zc2vpfpV7vJC8WEe9D`, tesouraria `Ex9pYvpA96dkZF5efdooEU2pQvWr5owpAZPiqXtLYxnh`, comissao 5000 bps. Assinatura de criacao informada pelo usuario: `2YcAUZwRzFBhFMsACpLueaRBRpRTEswrLNqWqSsaXGSgkEeNiUDeELSg9dger2ewvRn9VGnXbmX5Ky57b4MtZFRt`. Repeticao de configure foi rejeitada na simulacao por conta existente; configuracao mantida.

Oferta: `A2n5s6TYEH9oFZNFE1owxN5nEh18WGDHnEvrc5QvBgLR`, Skill original, preco 1000000000 lamports. Assinatura informada: `oASMhPRZB8aA1biQDRsVJeC5dyqA3dYH2QQxCSQ852RFmkw5ogBq7JtcUz9qJKZBtGvDjDJ9vpBBw3E67sZNsLd`. Consultas RPC antes da compra confirmaram owner JrSkill, preco e comissao.

Comprador CLI: `E5F1ztxZwgogcwWaryB6uCLXp3iZ2YA6MBzjwv6R51dE`. Cotacao --quote nao enviou transacao. Compra --send confirmou assinatura `4dwpsPA6GrZNhLuJhmdWQPJSv1r3QLnpMnxGrY4PvdQj2AAEzs2aeWu9YrBS7Kut5rhDgMenq9U7wiEd1GnMLKiY` e License PDA `5wjuV3W2fJo9o8U5oqfjcnbUULGxKqRxLXK9AgUvqADR`, verified=true.

Verificacao independente read-only: RPC Devnet genesis confirmado, transacao finalized no slot **507724452**, meta.err=null. Leitura da licenca valida owner/discriminator/PDA/comprador/Skill/oferta/modelo 0 e parcelas registradas. [Evidencia RPC arquivada](HACKATHON_DEVLOG/assets/day6/license-purchase-proof.json).

| Movimento na transacao | Lamports | SOL de teste |
| --- | ---: | ---: |
| Criador recebeu | 500000000 | 0,5 |
| Tesouraria recebeu | 500000000 | 0,5 |
| Deposito da License PDA | 1346200 | 0,0013462 |
| Taxa efetiva da rede | 5000 | 0,000005 |
| Debito total do comprador | 1001351200 | 1,0013512 |
| Saldo do comprador apos esta transacao | 3998648800 | 3,9986488 |

Configuracao, oferta e Skill mantiveram seus saldos; nenhum custo foi deduzido dos repasses do criador/tesouraria. Valores acima sao pre/post balances desta transacao, nao promessa de saldo futuro. Os fundos sao SOL de teste; nao receita comercial real.

Verificacao publica da Skill apos a compra: finalized no slot 507725504, 128 bytes, hash original, matches_checkpoint=true.

### Repeticao de A e compra independente de B

Usuario repetiu buy --send com A: cliente retornou `already_exists: true`, comprador A e mesma License PDA, sem assinatura nova. O fluxo do cliente consulta licenca e nao envia transacao nesse caso. Isso valida protecao do cliente, nao uma segunda compra submetida/rejeitada on-chain.

B `Dyf2qbLSn7pW7khP4z4xWykDr7Kuw5uMqmxzMFRuU6vs` foi consultada antes da compra: `exists: false`, PDA calculada `CdvpW2VoTK4E1H39d688g9mW9vFtLwasqXrdiVuModLo`. Depois de cotar e comprar a mesma Skill por 1 SOL, leitura confirmou `exists: true`, modelo 0 e parcelas 0,5/0,5. Assinatura: `oZkPZsQXtoYfC5FhjHjKS9zoyhtGsVkERdZaa6HKuBB4Y2x4Fuqif2TEa1ZErUmEsN8HZb5N3gmjk15nLNNd9jD`.

RPC independente confirmou finalized slot **507731090**, meta.err=null, licenca B valida e distinta da de A para a mesma Skill. Debito de B: **1001351200 lamports**; criador +500000000, tesouraria +500000000, deposito da licenca 1346200 e taxa efetiva 5000. [Pre/post balances e evidencia RPC de B](HACKATHON_DEVLOG/assets/day6/license-purchase-b-proof.json).

**Prova positiva A/B na Devnet concluida.** Duplicidade/rollback do contrato foram testados no validator local; nao sao alegados como novos testes negativos Devnet. Compra pela Phantom, listagem e enforcement de licenca no painel/robo continuam pendentes.

## Identificacao da entrega — correcao de versao

O upgrade comercial acima foi compilado/publicado com os metadados antigos `0.1.0`. A correcao seguinte identifica scripts npm e crate Rust como **0.2.0**, com seus lockfiles alinhados, e o painel como **JRBOT-PANEL-V1S-WALLET-07**. A identificacao do painel agora vem apenas de `app.APP_VERSION`, sem sobrescrita pelo wrapper stable.

Esta correcao nao altera instrucoes, layout de contas, JSON, firmware nem implementa compra no painel. A IDL gerada a partir do novo checkout recebe versao 0.2.0; a IDL enviada no upgrade anterior ainda tem os metadados 0.1.0. Nao se alega publicacao dessa correcao de metadados na Devnet. O programa comercial ja publicado atende os mesmos scripts/instrucoes; mudar o numero exibido no painel nao exige repetir o deploy.

Depois de git pull, feche o painel antigo e reabra **PAINEL.bat** para carregar WALLET-07. No WSL, npm ci/npm test mostram 0.2.0; anchor build gera a IDL local correspondente. Recarregue a pagina do painel com Ctrl+F5.

## Etapa 4D — compra no navegador

O painel **JRBOT-PANEL-V1S-PURCHASE-08** implementa cotacao, assinatura Wallet Standard Devnet, envio verificado e consulta finalized da licenca para a Skill conhecida. Testes de software passaram; a compra com Phantom real ainda precisa do teste do usuario. Nenhuma nova compra/deploy Devnet foi realizada neste checkpoint. O executor experimental continua independente da licenca. [Arquitetura, limites e passo a passo](JRSKILL_WALLET_PURCHASE_TEST.md).

### Resultado real posterior — PURCHASE-11 / log (78)

Compra pela Phantom confirmada, transacao finalized slot 507765827, meta.err=null. Carteira `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` recebeu licenca `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`. Preco 1 SOL de teste dividido 0,5/0,5; deposito 0,0013462; taxa efetiva 0,00008; debito total 1,0014262. Licenca/repasses verificados por RPC independente. Usuario confirmou execucao fisica local/Devnet, tambem result=ok no log; enforcement por licenca continua pendente. [Evidencias e limites](JRSKILL_WALLET_PURCHASE_TEST.md#resultado-real--phantom-e-execucao-fisica-log-78).
