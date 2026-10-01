# Etapa 3 — Skill da Solana para o executor JrBot

Data: 01/10/2026. Branch exclusiva: `V1s-00`. Issue: [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Objetivo e arquitetura

Executar no OLED o mesmo JSON de 128 bytes publicado na Etapa 2, agora obtido da PDA em vez do arquivo local da Skill.

```text
Botao Executar Skill da Devnet
  → GET /jrskill/skill-devnet no servidor local Python
  → getGenesisHash + getAccountInfo(finalized, base64) na RPC Devnet
  → validar envelope Anchor/Borsh, authority, schema e SHA-256
  → JSON recuperado entregue ao executor JavaScript existente
  → Recipe face_sequence resolvida na biblioteca local existente
  → Runtime API 1.1 pela Serial
  → OLED fisico
```

O adaptador [solana_skill.py](../tools/jrbot_frontend/solana_skill.py) usa somente a biblioteca padrao Python, sem wallet, Node, Anchor ou npm. O ESP32 nao acessa a Solana. A rota retorna texto UTF-8 preservado e metadados fora do payload; o executor interpreta o JSON com as funcoes existentes. O fluxo local continua disponivel em um botao separado.

Esta prova usa exclusivamente o checkpoint congelado:

- Programa: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`.
- PDA: `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`.
- Authority: `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij`.
- SHA-256: `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`.

Validacoes: genesis completo da Devnet, conta existente nao executavel, owner esperado, conta de 589 bytes, discriminator `account:Skill`, authority, schema 1, limites Borsh, payload de 128 bytes e hash armazenado/recalculado/checkpoint iguais. O parser tambem exige JSON v1 e `run` nao vazio. As chamadas continuam limitadas pelo executor existente a `face`, `wait` e `recipe`, com descoberta de capabilities e confirmacao da expressao pela Runtime API.

Cada clique consulta a rede novamente. Nao ha cache nem fallback para Skill local. Falhas de RPC/rede/validacao devolvem HTTP 502 e interrompem o fluxo antes de enviar chamadas `face`. A RPC tem timeout por consulta (10s genesis, ate 15s conta), e o painel mantem seu timeout de requisicao de 35s. A disponibilidade da RPC publica pode variar.

A biblioteca de Recipes continua local: somente a Skill principal veio da Solana. Nao houve alteracao do contrato, schema, JSON original, Recipe ou firmware. Licenciamento permanece na Etapa 4.

## Identificacao e logs

Painel atual: **`JRBOT-PANEL-V1S-SOLANA-03`**. A prova fisica inicial usou a versao 02; a versao 03 adiciona o indicador de disponibilidade. Firmware: **`JrBot_V1S_00`**, sem nova build.

Logs esperados (slot varia):

```text
JR_SKILL_SOLANA fetch=started
JR_SKILL_SOLANA source=solana-devnet pda=8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux rpc_slot=... payload_bytes=128 payload_hash=416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f hash_verified=true matches_checkpoint=true
JR_SKILL_V1 start=v1 source=solana-devnet top_calls=4
JR_SKILL_V1 recipe_enter=face_sequence depth=1
JR_SKILL_V1 recipe_exit=face_sequence depth=1
JR_SKILL_V1 result=ok source=solana-devnet
```

`result=ok` significa que as respostas da Runtime API foram confirmadas pelo painel. A prova fisica exige observar o OLED e registrar isso separadamente.

## Validacao executada pelo agente

- Cinco testes Python passaram: bytes exatos/commitment, rede errada, conta/metadados adulterados, timeout sem fallback e rota HTTP com os wrappers reais de `run_panel.py` (sucesso/HTTP 502).
- Dois testes Node passaram: executor com transporte Serial simulado produz `happy → surprised → thinking → happy → neutral`, resolve a Recipe local e nao le o arquivo local da Skill; falha RPC/prova invalida/capability ausente nao envia `face`.
- Consulta real pelo adaptador Python confirmou `finalized`, slot `506320351`, 128 bytes e hash/checkpoint corretos.
- **Teste fisico positivo confirmado pelo usuario em 01/10/2026.** O [log completo](HACKATHON_DEVLOG/assets/day4/jrskill-devnet-physical-log.txt) registra leitura Devnet no slot `506352693`, 128 bytes/hash correto, `source=solana-devnet`, respostas Runtime API para `happy → surprised → thinking → happy → neutral` e `result=ok`. A Recipe continua local. O usuario confirmou a execucao no robo; o resultado posterior do teste sem Internet esta registrado abaixo. Nao houve novo deploy; o build de firmware tentado pelo usuario falhou e sua correcao no instalador possui apenas validacao estatica.

Comandos de reproducao, a partir da raiz do repositorio:

```bash
python -m unittest discover -s tools/jrbot_frontend/tests -p test_solana_skill.py -v
node --test tools/jrbot_frontend/tests/executor-devnet.test.mjs
python tools/jrbot_frontend/solana_skill.py
```

Node e necessario somente para o teste JavaScript de desenvolvimento; o painel e a consulta Python nao dependem dele.

## Resultado do teste sem Internet — 01/10/2026

O [log original da tentativa offline](HACKATHON_DEVLOG/assets/day4/jrskill-devnet-offline-log.txt) registra, as 15:06:59, `fetch=started` e erro DNS `[Errno 11001] getaddrinfo failed`, seguido de `result=error source=solana-devnet`. Nao ha inicio de Skill, comando de face, Recipe ou `result=ok` nesse arquivo. **Bloqueio antes da execucao confirmado no log**, sem fallback bem-sucedido para Skill local. Falta confirmacao visual explicita de que o OLED ficou parado e o log da tentativa apos restaurar a Internet. A execucao fisica positiva anterior permanece validada separadamente.

## Recuperacao e indicador de disponibilidade

O usuario forneceu o [log completo da recuperacao](HACKATHON_DEVLOG/assets/day4/jrskill-devnet-recovery-log.txt) e confirmou o teste fisico, incluindo execucao local com/sem Internet. A tentativa Devnet de 15:06:59 falha; a de 15:09:43 recupera 128 bytes no slot `506359801` e termina com `result=ok source=solana-devnet` as 15:09:48. A execucao local termina com sucesso as 15:10:07; tentativas Devnet de 15:10:09/18 falham. A de 15:10:23 recupera a Skill novamente, mas o arquivo termina antes da ultima confirmacao. A primeira recuperacao completa valida o retorno da execucao online.

Na versao 03, `/jrskill/devnet-status` consulta `getGenesisHash` com timeout de 5s e valida a rede. O painel verifica ao abrir, repete a cada 30s enquanto nao estiver ocupado, e oferece **Verificar Devnet**. O estado da Solana fica separado da Serial: um erro de RPC/rede nao significa que o robo USB esteja desconectado. Skill local e controles Serial seguem disponiveis. O botao Devnet e liberado somente apos consulta positiva; cada execucao continua lendo/validando o payload novamente. Falhas de leitura atualizam o indicador; a verificacao periodica/manual permite recuperar o botao quando a RPC retorna.

Verificacoes da nova versao: **6 testes Python e 3 JavaScript passaram**, com transicoes de disponibilidade e execucao local offline; consulta real de disponibilidade confirmou a Devnet. A nova interface ainda aguarda teste do usuario. Sem alteracao de firmware, JSON ou contrato.

## Roteiro do teste fisico

1. Atualizar a **copia usada pelo painel**, estando na `V1s-00`: `git pull --ff-only origin V1s-00`.
2. Fechar o painel antigo para liberar a porta 8765 e a Serial. Abrir o painel atualizado pelo inicializador habitual ou, na raiz do repositorio: `python tools/jrbot_frontend/run_panel.py`.
3. Usar o ambiente Windows ja validado para o painel Serial, pois esta interface seleciona portas `COM`. O WSL pode rodar a consulta Python e os testes, mas esta interface nao enumera `/dev/tty*`.
4. Confirmar `JRBOT-PANEL-V1S-SOLANA-03` no painel e aguardar **Solana Devnet: disponivel**. Conectar a COM do JrBot e confirmar firmware/HW04/OLED disponivel.
5. Limpar o log para separar esta prova da execucao local. Clicar **Executar Skill da Devnet**.
6. Observar `happy → surprised → thinking → happy → neutral` no OLED. Confirmar no log a origem `solana-devnet`, PDA/hash corretos, Recipe local e `result=ok`.
7. Baixar o log TXT e registrar imagem do painel/video do OLED. Enviar os arquivos para fechar o checkpoint na Issue #33 e no Day 4.
8. Prova negativa na versao 03: com a Serial ainda conectada, interromper a Internet do PC e clicar **Verificar Devnet** (ou aguardar a verificacao automatica). Esperado: indicador indisponivel e botao Devnet bloqueado, mantendo o botao local disponivel. Se uma execucao Devnet for iniciada antes de o indicador detectar a queda, ela deve falhar sem `start`/`result=ok` nem mudanca de face. Restaurar a Internet, clicar **Verificar Devnet** e repetir a prova positiva. O teste nao pode executar a Skill local silenciosamente.

Nao e necessario gravar novamente a ESP32 ou executar `anchor build/deploy`. A prova usa o programa e a PDA ja publicados.

## Referencias

- [RPC getAccountInfo](https://solana.com/docs/rpc/http/getaccountinfo): consulta publica com commitment e encoding definidos.
- [RPC getGenesisHash](https://solana.com/docs/rpc/http/getgenesishash): identificacao do cluster.
- [Etapa 2](JRSKILL_SOLANA_STAGE2.md).
- [Day 4](HACKATHON_DEVLOG/day4.md).
