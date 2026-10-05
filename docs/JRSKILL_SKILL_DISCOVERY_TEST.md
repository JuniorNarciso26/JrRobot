# Busca das Skills da carteira — SKILLS-14

05/10/2026, exclusivamente V1s-00. Versao: **JRBOT-PANEL-V1S-SKILLS-14**. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Escopo

Layout anterior preservado. Somente o bloco Minha Skill de teste recebeu o titulo Minhas Skills, botao Buscar minhas Skills, estado da busca e lista. Busca automatica apos autenticacao, atualizacao manual e nova consulta quando uma compra passa a apresentar licenca. Trocar/desconectar carteira ou perder autenticacao limpa a lista e descarta respostas atrasadas. Falha RPC aparece como busca nao confirmada, nunca como carteira sem licencas.

Esta entrega implementa descoberta, sem seletor de execucao geral. Compra e autorizacao/execucao continuam no checkpoint minimal_recipe_01. Outras Skills validas sao listadas como licenciadas com execucao ainda nao habilitada. A lista nao emite permit, nao envia comando ao robo e nao substitui a consulta de licenca antes de cada comando do fluxo existente. Sem contrato/firmware/JSON/Recipe novos ou deploy.

## Arquitetura

GET /jrskill/wallet/skills usa a carteira da sessao autenticada, sem aceitar comprador informado na URL. Autenticacao e comprador sao conferidos antes e depois da consulta. Confirmar genesis Devnet; getProgramAccounts finalized/withContext filtra License de 137 bytes pelo discriminator e comprador no offset 9. Revalidar owner, discriminator, versao 00, comprador, PDA derivada, oferta derivada, valores pagos e ausencia de duplicatas no servidor.

Carregar Skills/ofertas vinculadas em getMultipleAccounts finalized, com minContextSlot da descoberta. Validar owner/tamanho, schema 1, SHA-256 dos bytes, PDA [skill, authority, hash], JSON v1 e relacao Skill/oferta/criador/preco. Sem cache/fallback local. Limite de 32 licencas por busca nesta prova; excesso ou resposta RPC acima de 64 KiB retorna erro explicito, sem truncar a lista nem alegar resultado vazio. Nao ha paginacao/indexador nesta entrega.

O schema atual nao tem nome/descricao: o checkpoint conhecido usa minimal_recipe_01; outros resultados recebem Skill + prefixo do endereco. Enderecos/hash ficam em Ver Skill e licenca. Nao inferir nome comercial do payload.

Referencia primaria: [Solana getProgramAccounts](https://solana.com/docs/rpc/http/getprogramaccounts). Layouts e seeds conferidos no programa JrSkill atual da V1s-00.

## Verificacao realizada

- **39 testes Python passaram:** inclui duas Skills de criadores diferentes para um comprador, filtros, vinculos/PDA/hash/owner, duplicata/limite, rede incorreta, falha RPC, resposta vazia e sessao trocada durante HTTP.
- **29 testes JS passaram:** 24 de carteira (inclui busca, atualizacao, autenticacao e resposta atrasada) e 5 do executor existente. Bundle recompilado.
- **Leitura real Devnet, sem transacao:** carteira licenciada 6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R retornou minimal_recipe_01/licenca 7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45, hash verificado, finalized slot 507816052. Carteira sem licenca 8zQwpe3qVBzoPMasLQSbeagtmKzqL1JLG4iEApznUySo retornou lista vazia no slot 507816055.
- Navegador local confirmou layout anterior, SKILLS-14, bloco Minhas Skills e busca bloqueada sem carteira. Sem Phantom conectada nem robo na verificacao: teste real da extensao/hardware desta entrega ainda nao realizado. Nao houve compilacao/deploy de contrato ou firmware.

## Teste no painel

1. Fechar servidor. No checkout Windows do PAINEL.bat, confirmar V1s-00 e executar git pull --ff-only origin V1s-00. Abrir PAINEL.bat e Ctrl+F5; conferir SKILLS-14. Nao precisa npm/Anchor, gravar firmware, deploy ou comprar novamente.
2. Conectar/autenticar a carteira que comprou: Minhas Skills deve listar minimal_recipe_01. Buscar minhas Skills repete a leitura. Abrir Ver Skill e licenca para conferir PDAs.
3. Trocar para carteira sem licenca e autenticar: lista anterior deve desaparecer; busca concluida deve informar ausencia de Skills licenciadas neste programa na Devnet. Erro de rede deve informar busca nao confirmada.
4. Execucao da Skill de teste permanece no botao existente, exigindo a carteira licenciada e Serial. Descoberta nao executa nada automaticamente.

Proxima etapa a discutir: selecao/execucao de Skills descobertas e transporte do fluxo para o app do robo. Marketplace fica na plataforma web, painel permanece desenvolvimento.
