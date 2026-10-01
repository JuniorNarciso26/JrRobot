# Etapa 4 — licenciamento com License PDA

## Decisao de arquitetura — 01/10/2026

O usuario escolheu manter o modelo de **License PDA**, em vez de NFT, para permitir regras proprias de licenciamento e evolucao do marketplace JrSkill. Esta decisao foi registrada na branch `V1s-00` e na [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**Estado: arquitetura escolhida; licenciamento e conexao de carteira ainda nao implementados.**

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
- Emissao de teste inicialmente e, depois, compra com pagamento e entrega atomicos.
- Prova de controle da wallet por assinatura e descoberta/validacao das licencas no painel.

A escolha de PDA nao implementa automaticamente pagamento, revenda, expiracao ou recuperacao. Essas regras precisarao de desenho, codigo e testes. A autenticacao da wallet nao pode se limitar a informar um endereco publico.

## Proxima prova

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
