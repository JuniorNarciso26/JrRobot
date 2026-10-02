# Etapa 4C — confirmar RPC Devnet e saldo da carteira

## Implementacao — 02/10/2026

Painel **JRBOT-PANEL-V1S-WALLET-06**, exclusivamente na `V1s-00`. Esta prova e somente de leitura; nao instala rede, solicita airdrop, assina transacao ou compra licenca.

Devnet e uma rede de teste existente. Na Phantom, o usuario ativa **Settings → Developer Settings → Testnet Mode → Solana Devnet**. O painel inclui essas instrucoes e o [guia oficial](https://help.phantom.com/articles/use-testnets-in-phantom-5997313271699).

## O que e confirmado

- A carteira e a conta anunciam suporte a `solana:devnet` no Wallet Standard.
- O RPC fixo do painel e a Devnet: exige o genesis hash completo `EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG` antes de consultar saldo.
- Saldo desse endereco na Devnet, via `getBalance` com commitment finalized, retornando slot e lamports. Valores inteiros viram strings para evitar perda de precisao JavaScript.
- O endereco vem da sessao autenticada no servidor. Cliente nao escolhe outro endereco na rota; sessao e conferida novamente depois da leitura RPC.

**Nao e confirmado:** qual rede esta selecionada na interface da extensao. O campo Wallet Standard `chains` representa redes suportadas, nao a rede atualmente exibida na Phantom. A UI e os logs deixam isso explicito (`extension_network=unknown`). O usuario deve conferir Testnet Mode/Solana Devnet dentro da propria carteira. Nao presumir um metodo universal de troca de rede nem simular uma troca como confirmada.

## Comportamento

Depois de autenticar, o painel consulta automaticamente a rota local GET `/jrskill/wallet/devnet`. Tambem existe **Verificar Devnet e saldo** para atualizar manualmente. A rota exige sessao autenticada valida; sem ela responde 401.

RPC incorreto, offline ou resposta malformada mostram **nao confirmado**, sem transformar erro em saldo zero. Troca de carteira, perda de autenticacao ou desconexao apagam o resultado; respostas atrasadas para outra conta sao descartadas.

**0 SOL de teste** e uma leitura valida, nao ausencia de Devnet instalada. Nao basta consultar saldo para afirmar que a carteira pode pagar uma compra futura; preco, rent e taxas serao tratados na prova comercial 00.

## Evidencia de autenticacao do usuario — log (73)

[Original](HACKATHON_DEVLOG/assets/day5/jrskill-wallet-auth-log.txt), SHA-256 `291f1e247b6698057730017ef65bcb47f755fac63292bc58b8d2fe4bef35d445`, preservado byte a byte.

- 13:24:43: conectada, carteira `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R`.
- 13:25:01: autenticada; evento do navegador com expiracao e evento do servidor confirmando verificacao.
- As duas linhas sao registros das duas camadas do mesmo fluxo, nao duas compras ou transferencias.

Horarios do painel sem timezone no arquivo. O log confirma autenticacao; nao registra rede selecionada na extensao, saldo, compra ou execucao fisica. O usuario relatou conexao bem-sucedida e pediu a prova de rede seguinte.

Consulta real, somente leitura, feita pelo agente ao preparar esta entrega: genesis Devnet correto, saldo **0 lamports**, slot finalized **506696407** para esse endereco. Este resultado e um checkpoint, nao garantia de saldo futuro nem de rede selecionada na extensao.

## Teste do usuario

1. Fechar painel antigo; atualizar V1s-00 com `INSTALAR.bat panel`.
2. Abrir no Chrome e confirmar WALLET-06; Ctrl+Shift+R se necessario.
3. Na Phantom, ativar Testnet Mode e escolher **Solana Devnet**. Nao escolher Solana Testnet, que e outra rede.
4. Conectar e autenticar a carteira de teste.
5. Conferir **RPC: Solana Devnet confirmada**, suporte anunciado e saldo **SOL de teste**. Clicar Verificar Devnet e saldo para repetir a leitura.
6. Desconectar ou trocar conta e conferir que o saldo/resultado anterior desaparece. Reautenticar para nova consulta.
7. Baixar log TXT e registrar a selecao de Devnet observada na propria Phantom. O log RPC sozinho nao comprova essa selecao.

## Validacao e limites

- 3 testes Python novos: saldo finalized/precisao, genesis incorreto sem leitura subsequente e erro offline/saldo invalido distinto de zero.
- 2 testes JS novos: consulta apenas autenticada, saldo zero valido, resposta atrasada/conta errada e erro RPC sem falso sucesso.
- Teste HTTP de autenticacao ampliado: rota de rede exige autenticacao, usa endereco da sessao e rejeita resultado se a sessao for revogada durante a consulta.
- Interface Chrome headless com carteira simulada: assinatura verificada no servidor Python real, RPC Devnet real, saldo zero apresentado; troca de conta/recusa/retry/desconexao passaram sem erro JS. Nao e teste da configuracao visual da Phantom real.

Pendente: teste WALLET-06 no computador do usuario e confirmacao visual da configuracao Devnet na Phantom. Sem alteracao de firmware, contrato, JSON/Recipe, build ou deploy Solana. Compra e licenca continuam nao implementadas.

Referencias: [Wallet Standard — campos chains](https://github.com/wallet-standard/wallet-standard/blob/master/packages/core/base/src/wallet.ts), [Etapa 4B](JRSKILL_WALLET_AUTH_TEST.md), [modelo comercial 00](JRSKILL_LICENSE_MODEL_00.md).
