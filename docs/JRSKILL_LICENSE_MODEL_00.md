# JrSkill — prova de compra de licenca, modelo 00

## Escopo e estado — 02/10/2026

Desenho da proxima prova isolada na `V1s-00`, conforme a [Issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46). A [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33) continua com integracao, autenticacao no painel e execucao fisica.

**Estado: arquitetura proposta e plano de teste; compra e License PDA ainda nao implementadas.** Este documento nao representa build, upgrade, deploy ou teste realizado.

O escopo comercial 00 continua sendo compra, repasse e registro da licenca. Atualizacao de ordem em 02/10/2026: por decisao do usuario, antecipamos uma prova somente de conexao da carteira no painel, documentada na [Etapa 4A](JRSKILL_WALLET_CONNECTION_TEST.md). Ela nao implementa compra ou licenciamento. A numeracao comercial 00/01/02 e independente do schema JSON v1 e do firmware JrBot_V1S_00.

## Desenho minimo proposto

Manter um unico programa JrSkill e preservar o layout da Skill existente. Nenhuma nova instrucao deve modificar seu payload, authority, schema ou hash.

| Entidade | Responsabilidade |
| --- | --- |
| Skill existente | Conteudo e identificacao do criador. |
| Configuracao comercial 00 | Carteira de comissao JrBot e percentual do teste. |
| Oferta 00 | Skill, criador, preco em lamports e termos da divisao. |
| License PDA | Carteira compradora, Skill, modelo 00 e registro da compra. |

Derivacao proposta da licenca: `["license", comprador, skill_pda]`. A e B podem comprar a mesma Skill sem duplicar o JSON. Uma carteira nao recebe duas licencas da mesma Skill nesta prova.

A configuracao comercial precisa de inicializacao restrita a uma autoridade JrBot verificada pelo programa; nao basta deixar o primeiro chamador escolher a carteira que recebe a comissao. A oferta exige a assinatura do criador correspondente a `Skill.authority`. Endereco de recebedor informado pelo cliente nao substitui essa verificacao.

Para simplificar a 00, propor configuracao e termos da oferta imutaveis durante a prova. Atualizacao de preco/comissao, retirada de venda e migracao ficam para discussao posterior. A configuracao comercial nao deve alterar quem pode publicar uma Skill pelo fluxo atual.

## Compra proposta

```text
Comprador consulta oferta e aceita seus termos
  -> assina compra com limite de preco aceito
  -> programa valida Skill, oferta, recebedores e assinatura
  -> distribui o preco para criador e JrBot
  -> registra License PDA do comprador
  -> cliente confirma transacao e consulta a licenca
```

Pagamento e emissao devem ocorrer na mesma transacao. A licenca fica vinculada a Skill e, portanto, ao conteudo adquirido. Uma falha nao pode deixar pagamento parcial ou licenca criada sem o pagamento exigido. Taxas de rede sao distintas e podem existir mesmo em transacoes que falham.

O comprador proposto paga preco, taxa da transacao e deposito necessario para a conta da licenca. Essa responsabilidade e os valores devem ser mostrados separadamente antes de assinar; ainda precisam ser fechados como regra do teste na #46.

Calculos em lamports inteiros, com verificacao de overflow e percentual limitado a 0..10000 basis points. Proposta de arredondamento: comissao = piso(preco * percentual / 10000); criador = preco - comissao. Assim as duas parcelas somam exatamente o preco.

Preco de 1 SOL de teste e comissao de 5000 basis points (50%) sao exemplos, nao tabela definitiva. Os scripts devem receber termos explicitamente e exibir a divisao antes da compra. Nao deve existir compra automatica ao abrir o painel ou consultar uma oferta.

Antes de enviar, o cliente pode detectar uma licenca existente e apenas consulta-la. O programa tambem deve impedir compra duplicada, inclusive quando dois clientes tentam comprar simultaneamente. Uma segunda tentativa enviada a rede pode pagar taxa de rede, mas nao pode pagar novamente o preco da licenca.

## Prova em duas fases

### 1. Validator local

Usar carteiras descartaveis distintas para administracao, criador, comissao JrBot, comprador A e comprador B. Provar:

- criador autorizado cria oferta; outro signer nao pode vender aquela Skill;
- configuracao comercial nao pode ser capturada por um chamador arbitrario;
- A compra: parcelas corretas, licenca com comprador/Skill/modelo corretos;
- B nao tem licenca; ao comprar recebe outra PDA para a mesma Skill;
- duplicidade nao repete o pagamento;
- preco acima do limite aceito, recebedor incorreto, saldo insuficiente ou oferta/Skill invalida nao deixam pagamento ou emissao parcial;
- uma falha depois de alguma operacao de pagamento tambem reverte os efeitos da compra;
- rejeitar overflow e percentual invalido; conferir arredondamento;
- a Skill publicada antes da extensao continua sendo lida com bytes e hash identicos.

Comparar parcelas usando recebedores que nao pagam a taxa da transacao. Separar deposito da License PDA e taxa da rede da variacao de saldo do comprador. Registrar tambem o saldo e a ausencia de conta nos cenarios de rollback.

### 2. Solana Devnet

Somente depois da compilacao e testes locais: preservar a chave original do programa, verificar compatibilidade e preparar o upgrade do mesmo Program ID. Este documento nao executa nem confirma esse upgrade.

Os clientes devem confirmar o genesis hash da Devnet antes de qualquer escrita. Mainnet fica fora desta prova. Usar carteiras locais de teste; nunca versionar suas chaves privadas.

Publicar os termos da prova e registrar uma compra com assinatura, enderecos publicos, preco, parcelas, deposito, taxa e License PDA. A consulta publica precisa conferir owner, tipo da conta, comprador, Skill e modelo, nao apenas a existencia de qualquer conta naquele endereco.

## Entregaveis da implementacao seguinte

- Instrucoes e contas comerciais minimas no programa existente.
- Scripts separados para configurar a prova, criar a oferta, comprar e consultar licenca.
- Testes locais de compra, repasse, permissao, duplicidade e rollback.
- Relatorio de evidencias que separe analise estatica, compilacao, teste local e Devnet.

Sem carteira no painel, alteracao de firmware, novas Recipes, mudanca no JSON v1, NFT, revenda ou expansao do marketplace nesta primeira prova. A futura verificacao de licenca no painel nao torna o JSON publico impossivel de copiar.

## Evolucao

Fechar resultados da 00 na #46 antes de definir a 01. Registrar o efeito de qualquer nova regra sobre compradores anteriores. Conexao e autenticacao de carteira, descoberta de licencas e autorizacao antes das acoes fisicas seguem na [Etapa 4](JRSKILL_SOLANA_STAGE4.md), depois da compra isolada.
