# JrSkill — App embarcado / Minhas Skills

**Branch:** `V1s-00`  
**Candidata:** `JrBot_V1S_APP_03`
**Data:** 2026-10-07

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

## APP_03 / painel SKILLS-15 — expressoes e selecao generica

O teste fisico de jrteste revelou uma allowlist limitada a quatro faces no App.
Esta candidata alinha o App e o autorizador do painel com as 16 expressoes do modulo OLED,
incluindo sad e worried. battery_low passa a ser aceito como nome canonico pelo firmware.
O App valida o documento inteiro, incluindo recipes, antes do primeiro comando Runtime API.

O painel permite selecionar qualquer Skill JSON v1 licenciada descoberta na Devnet.
A leitura valida owner, discriminator, schema, tamanho, hash e PDA derivado de autor/hash.
A permissao de execucao fica vinculada a sessao, comprador e PDA escolhido; uma leitura
finalized da licenca antecede cada comando. A referencia permanece como fallback
apenas para chamadas legadas sem parametro skill; a interface exige selecao explicita.
Outros schemas continuam recusados; compras permanecem na Store.

Teste do usuario:
1. Atualizar V1s-00 e instalar JrBot_V1S_APP_03 pelo INSTALAR.bat.
2. Confirmar firmware APP_03 e painel JRBOT-PANEL-V1S-SKILLS-15.
3. No App, recarregar, autenticar a carteira compradora e buscar as duas Skills.
4. Selecionar Skill 8AgpL6dM e executar: happy → sad → thinking → neutral → worried.
5. No painel, autenticar, buscar Skills e escolher o mesmo PDA no seletor antes de executar.
6. Repetir com minimal_recipe_01 para verificar compatibilidade.

VALIDACAO AUTOMATIZADA: testes Python de leitura/licenca/permissoes e testes Node
de App, executor, autenticacao e descoberta. TESTE FISICO APP_03: pendente.

COMPILACAO APP_03: concluida em 07/10/2026 com ESP-IDF 5.5.5, ESP32-S3 e
configuracao sdkconfig.develop-v1.6.3 do HW04. Binario 0x217740 bytes,
58% livres na particao de aplicacao. Testes: 43 Python + 33 JavaScript
+ 6 verificacoes da politica do painel. Leitura real de jrteste na Devnet
confirmou 170 bytes, hash verificado e as cinco faces; nenhum comando fisico
foi enviado nessa verificacao. Instalacao/teste fisico APP_03: pendentes.
