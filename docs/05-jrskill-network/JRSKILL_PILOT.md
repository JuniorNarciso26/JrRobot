# Piloto externo JrSkill — crie uma Skill e veja no robô

O piloto interno foi concluído em 07/10/2026: `jrteste` foi criada na Store,
comprada por outra carteira e executada no App e no painel do JrBot.
Agora buscamos os primeiros **3 colaboradores externos**. Isso é uma meta,
não uma contagem de participantes já confirmados.

Você pode participar com ou sem JrBot. Sem hardware, o mantenedor faz o teste.
Ponto de entrada e respostas: [convite no GitHub #53](https://github.com/JuniorNarciso26/JrRobot/issues/53).

## Caminho A — publicar pela Store

1. Abra [Crie sua Skill](https://jrbot.com.br/pt/store/create/) no navegador com Phantom.
2. Conecte e autentique a carteira. A autenticação é uma assinatura de mensagem.
3. Use Solana Devnet e solicite SOL de teste no [Faucet](https://faucet.solana.com/).
4. Preencha nome, descrição, preço em test SOL e uma sequência original de 3 a 8 expressões.
5. Para esta primeira rodada, use `happy`, `sad`, `surprised`, `thinking`, `worried` e `neutral`,
   já exercitadas nos testes físicos. Prefira esperas de 500 a 1000 ms e termine em `neutral`.
6. Prepare a cotação, confira os custos, aceite e assine a publicação na Phantom.
   Aguarde a confirmação; guarde o link da Skill e da transação.
7. Confira sua Skill no [catálogo](https://jrbot.com.br/pt/store/) e envie o link na issue #53.

Limites: JSON v1, payload de até 512 bytes; até 32 etapas na interface, com
espera inteira de 0 a 5000 ms. O tamanho em bytes pode limitar a sequência antes
das 32 etapas. O piloto usa somente expressões e esperas, sem recipes externas.

Ao publicar com sua carteira, ela é o creator on-chain. O preço e a divisão
creator/JrBot aparecem na cotação de compra. Tudo ocorre em test SOL na Devnet.

## Caminho B — contribuir sem carteira ou sem publicação própria

Envie na issue #53 o nome, a descrição, o crédito desejado e um JSON original:

```json
{"v":1,"run":[["face","happy"],["wait",500],["face","worried"],["wait",500],["face","neutral"]]}
```

Esse JSON é apenas um exemplo. Crie uma sequência diferente para participar.
O mantenedor revisa e pode publicar por sua carteira. Nesse caminho, **o creator
on-chain é a carteira publicadora do mantenedor**; seu crédito como autor fica
registrado na submissão e na demonstração. Para ser o creator on-chain, use o caminho A.

## Testar com ou sem robô

**Com JrBot:** use a linha `V1s-00`, firmware `JrBot_V1S_APP_03` ou uma sucessora
documentada que suporte esse piloto, e painel `JRBOT-PANEL-V1S-SKILLS-15`.
Use uma carteira compradora diferente da creator para adquirir a licença na Store.
No App ou painel: autenticar → confirmar Devnet → buscar Skills → selecionar → executar.
Envie a versão exata, resultado, log e, se possível, vídeo.

**Sem JrBot:** depois da publicação, o mantenedor adquire/confirma a licença com
a carteira de teste e executa a Skill no robô. Não é necessário comprar uma licença
para apenas submeter sua criação. O mantenedor responde com aprovação, erro ou
pedido de ajuste, e registra a evidência física quando o teste acontecer.

## O que enviar e como acompanhamos

Na [issue #53](https://github.com/JuniorNarciso26/JrRobot/issues/53), informe:

- nome e link/PDA da Skill, ou JSON para publicação assistida;
- comportamento esperado e se você possui JrBot;
- crédito desejado e autorização, ou não, para publicar uma demonstração;
- resultado/log com versão, se testou, e dificuldades encontradas.

O registro de cada contribuição segue: recebida → validada → publicada → licenciada
→ teste físico → evidência e feedback. O mantenedor confirma cada etapa separadamente;
uma criação na blockchain não é, por si só, teste físico aprovado.

Depois do primeiro colaborador, ajustamos o roteiro pelo feedback e seguimos com
os próximos dois. A expansão do convite vem após essa rodada, com resultados reais.

## Texto para compartilhar

> Crie uma Skill e veja sua ideia rodar em um robô real! Estamos buscando os primeiros
> colaboradores do JrSkill Network. Monte uma sequência de expressões, publique na
> Solana Devnet ou envie um JSON. Não precisa ter JrBot: nós testamos no robô e
> registramos o resultado com seu crédito. É um piloto com SOL de teste.
> Participe: https://github.com/JuniorNarciso26/JrRobot/issues/53

> Build a JrSkill. We'll run it on a real robot! Join the JrSkill Network pilot:
> create an original OLED expression sequence, publish it on Solana Devnet or submit
> JSON for assisted publication. No robot required: we can test it on a physical
> JrBot and record the result with your chosen attribution. Test SOL only.
> Join: https://github.com/JuniorNarciso26/JrRobot/issues/53

Histórico: [issue #52](https://github.com/JuniorNarciso26/JrRobot/issues/52).
Detalhes do firmware/painel: [JRSKILL_APP.md](JRSKILL_APP.md).
