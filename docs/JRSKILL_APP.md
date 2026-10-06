# JrSkill — App embarcado / Minhas Skills

**Branch:** `V1s-00`  
**Candidata:** `JrBot_V1S_APP_02`  
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

Como o firmware foi alterado, builds instaladas nao sao reutilizadas.

Historico:

- `JrBot_V1S_APP_01`: instalou e exibiu a identificacao correta, mas o board **Minhas Skills** nao apareceu;
- `JrBot_V1S_APP_02`: corrige a entrega HTTP do JavaScript embarcado.

Nova candidata:

```text
JrBot_V1S_APP_02
```

A branch `V1s-00` usa o diretorio de build `build-v1s-app-02`.

## Estado

- **IMPLEMENTADO:** codigo na `V1s-00`;
- **COMPILACAO/INSTALACAO APP_02:** concluida no ambiente do usuario;
- **TESTE FISICO DO BOARD MINHAS SKILLS:** validado em 06/10/2026;
- **ACESSO:** usar o App por **HTTPS**. Em HTTP, recursos de seguranca do navegador/Wallet podem ser bloqueados.

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


## Correcao APP_02 — JavaScript embarcado

No primeiro teste fisico da `APP_01`, o App principal carregou e mostrou `JrBot_V1S_APP_01`, mas o board **Minhas Skills** nao foi criado.

A causa foi identificada no handler `/jrskill-app.js`: o arquivo foi incorporado com `EMBED_TXTFILES`, que gera uma string terminada em NUL. O handler enviava o intervalo inteiro entre os simbolos `_start` e `_end`, incluindo o terminador NUL no corpo JavaScript.

Um byte NUL no fonte JavaScript torna o script invalido no parser. A `APP_02` passa a servir o recurso como string usando:

```c
httpd_resp_send(req, jrskill_app_js_start, HTTPD_RESP_USE_STRLEN);
```

Assim o terminador NUL nao faz parte da resposta HTTP.

Estado da `APP_02`:

- **correcao implementada**;
- **compilacao/instalacao:** concluida pelo usuario;
- **teste fisico:** validado em 06/10/2026.

### Evidencia funcional APP_02

No App embarcado acessado por **HTTPS**, o usuario confirmou:

- board **Minhas Skills** visivel;
- **Wallet autenticada**;
- **Devnet confirmada**;
- saldo Devnet exibido;
- descoberta de `minimal_recipe_01` como licenciada;
- payload de **128 bytes**;
- hash exibido com prefixo `416d6af34ead...`;
- Skill selecionada;
- execucao concluida com **`result=ok`**.

A tentativa anterior em HTTP encontrou bloqueios do Chrome. Para esta fase, o fluxo Wallet/Devnet do App deve ser aberto em `https://<IP_DO_JRBOT>/`.
