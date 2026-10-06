# JrSkill — App embarcado / Minhas Skills

**Branch:** `V1s-00`  
**Candidata:** `JrBot_V1S_APP_01`  
**Data:** 2026-10-05

## Decisao

Nao criar um App novo do zero. O **App local que ja existe no firmware do JrBot** recebe um novo board chamado **Minhas Skills**.

O painel de desenvolvimento continua separado e permanece como ferramenta de laboratorio/diagnostico.

## Board Minhas Skills

A candidata leva para o App:

- **Conectar carteira**;
- **Autenticar carteira** por assinatura de mensagem;
- **Verificar Devnet** e saldo;
- **Buscar minhas Skills** licenciadas;
- selecionar uma Skill;
- executar a Skill selecionada no JrBot;
- desconectar a Wallet.

A descoberta usa o programa JrSkill ja publicado na Devnet e valida contas pertencentes ao Program ID, discriminator de License/Skill, comprador, schema e SHA-256 do payload.

Antes de cada acao fisica, a License account selecionada e consultada novamente.

## Compra nao vai para o App

O firmware **nao** recebe:

- consultar cotacao;
- aceitar preco;
- botao Comprar;
- assinatura de transacao de compra;
- marketplace.

A venda sera feita pela futura **Store web**.

```text
Store web
  -> comprar Skill
  -> License PDA
  -> App do JrBot
  -> Minhas Skills
  -> selecionar
  -> executar
```

## Onde roda a Solana

A decisao de arquitetura continua preservada: o **ESP32 nao vira cliente Solana**.

```text
navegador
  +-> Phantom
  +-> RPC Solana Devnet
  +-> valida License/Skill
  +-> /cmd no JrBot
        -> Runtime API
        -> hardware
```

O firmware serve o JavaScript do App; a comunicacao HTTPS com a Devnet acontece no navegador.

## Executor

Esta primeira candidata preserva o contrato ja validado:

- `face`;
- `wait`;
- Recipe local `face_sequence`.

Nao houve mudanca no JrSkill JSON v1 nem no programa on-chain.

## Identificacao

Como o firmware foi alterado, a build `JrBot_V1S_00` nao e reutilizada.

Nova candidata:

```text
JrBot_V1S_APP_01
```

A branch `V1s-00` usa o diretorio de build `build-v1s-app-01`.

## Estado

- **IMPLEMENTADO:** codigo na `V1s-00`;
- **VALIDACAO ESTATICA:** analise de codigo realizada; verificacao executavel ainda nao registrada;
- **COMPILACAO:** pendente; a primeira tentativa foi interrompida antes da compilacao por conflito no ambiente Python do ESP-IDF;
- **TESTE FISICO:** pendente.

## Primeiro teste

1. atualizar `V1s-00`;
2. rodar `INSTALAR.bat`;
3. confirmar no log `JrBot_V1S_APP_01`;
4. abrir o App local do JrBot no navegador com Phantom;
5. localizar **Minhas Skills**;
6. Conectar carteira;
7. Autenticar carteira;
8. Verificar Devnet;
9. Buscar minhas Skills;
10. escolher `minimal_recipe_01`;
11. Selecionar Skill;
12. Executar Skill no JrBot;
13. confirmar fisicamente `happy -> surprised -> thinking -> happy -> neutral`;
14. confirmar que nao existe fluxo de compra no App.
