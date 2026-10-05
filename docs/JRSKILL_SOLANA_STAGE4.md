# Etapa 4 — licenciamento com License PDA

## Decisao de arquitetura — 01/10/2026

O usuario escolheu manter o modelo de **License PDA**, em vez de NFT, para permitir regras proprias de licenciamento e evolucao do marketplace JrSkill. Esta decisao foi registrada na branch `V1s-00` e na [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**Estado atualizado em 05/10/2026: compras CLI A/B finalizadas na Devnet, com licencas independentes e repasses verificados. Compra pelo painel implementada e testada em software, com carteira/RPC simuladas; teste pela extensao Phantom real ainda pendente. Identificacao atual: JRBOT-PANEL-V1S-PURCHASE-08 e scripts/crate 0.2.0.** [Etapa 4A](JRSKILL_WALLET_CONNECTION_TEST.md), [autenticacao 4B](JRSKILL_WALLET_AUTH_TEST.md), [rede/saldo 4C](JRSKILL_WALLET_DEVNET_TEST.md), [compra pelo navegador 4D](JRSKILL_WALLET_PURCHASE_TEST.md) e [prova comercial 00](JRSKILL_LICENSE_MODEL_00_TEST.md). A consulta apresenta somente a Skill de teste conhecida; descoberta geral e autorizacao antes da execucao fisica continuam pendentes.

## Modelo proposto

- Skill PDA: conteudo executavel, schema, hash e publicador.
- License PDA: registro de autorizacao vinculado a uma wallet e a uma Skill.
- Programa JrSkill: instrucoes que controlam emissao e validacao das licencas.
- Painel: autentica a carteira, consulta as licencas e apresenta as Skills autorizadas.

Derivacao proposta, a confirmar no desenho detalhado:

```text
License PDA = ["license", wallet, skill_pda]
```

Uma mesma Skill pode ter licencas para A e B, sem duplicar o payload. A conta License PDA e um registro do programa, nao um NFT exibido automaticamente na carteira; a lista sera apresentada pelo painel JrSkill.

PDA e uma conta de dados, nao outro contrato executavel. Skill e License podem ficar no mesmo programa; um marketplace separado podera chamar instrucoes desse programa via CPI se necessario futuramente.

## Regras ainda a definir

- Quem pode emitir uma licenca e como o programa verifica essa autoridade.
- Licenca permanente ou com validade; possibilidade de revogacao.
- Transferencia entre wallets e eventual recuperacao de acesso.
- Direito a uma versao especifica da Skill ou a atualizacoes futuras.
- Compra de teste com pagamento e entrega atomicos: escopo comercial 00 discutido na Issue #46.
- Prova de controle da wallet por assinatura e descoberta/validacao das licencas no painel.

A escolha de PDA nao implementa automaticamente pagamento, revenda, expiracao ou recuperacao. Essas regras precisarao de desenho, codigo e testes. A autenticacao da wallet nao pode se limitar a informar um endereco publico.

## Proxima prova

Atualizacao de sequencia em 02/10/2026: o usuario decidiu antecipar a selecao/conexao de carteira no painel. A [Etapa 4A](JRSKILL_WALLET_CONNECTION_TEST.md) implementa somente conexao, sem assinatura, compra ou autorizacao de execucao. Depois seguimos com autenticacao e a prova isolada de compra, repasse e registro de License PDA da [Issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46). Desenho e plano de teste: [modelo comercial 00](JRSKILL_LICENSE_MODEL_00.md). Preco e divisao apresentados na issue sao exemplos para discussao.

A integracao abaixo vem depois da prova de compra:

```text
Conectar e autenticar wallet
  → listar licencas emitidas pelo programa JrSkill
  → conferir autorizacao da Skill selecionada
  → recuperar e validar o payload
  → executor existente → Runtime API → JrBot
```

Comparar uma wallet licenciada com outra sem licenca. A carteira sem autorizacao deve ser bloqueada antes de enviar acoes ao robo. O payload publico continua copiavel; o controle de licenca vale para o fluxo que aplica essas regras.

O contrato, o JSON v1, a Recipe e o firmware permanecem inalterados nesta decisao documental. Antes de upgrade, preservar o layout das Skills existentes e validar compatibilidade.

## Referencias do projeto

- [Etapa 3 validada](JRSKILL_SOLANA_STAGE3.md)
- [Diario Day 4](HACKATHON_DEVLOG/day4.md)
