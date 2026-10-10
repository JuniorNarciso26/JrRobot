# Documentação do JrBot

Este diretório contém a documentação **ativa** do JrBot e o histórico técnico preservado do MVP JrSkill Network/Colosseum. O estado das releases deve ser lido em conjunto com o `README.md`, `PROJECT_STATUS.md` e os registros de validação.

## Estado atual

- release oficial: **`JrBot_V1.7.04`** em `main`;
- integração: `develop`, atualmente na mesma baseline da release;
- marco JrSkill/Solana: **MVP concluído** em `V1s-00` (`JrBot_V1S_APP_03`, Solana Devnet);
- linha de voz local: `v2`;
- referência histórica da V1: `v1`.

A documentação ativa incorpora as fontes de evidência da V1S, incluindo o diário do Colosseum, decisões, testes e provas da integração. A branch histórica `V1s-00` também deve ser preservada. O MVP na Devnet foi comprovado, mas não equivale a produto Mainnet ou nova release estável homologada.

## Histórico do hackathon e JrSkill Network

**O histórico original deve permanecer acessível e nunca ser movido para `trash/` como documentação obsoleta.** O MVP da V1S alcançou seu objetivo em 2026-10-10; uma futura linha de desenvolvimento será definida separadamente.

- [Diário Colosseum — Kickoff e Day 1–9](HACKATHON_DEVLOG.md) — cronologia do desenvolvimento;
- [Day 8](HACKATHON_DEVLOG/day8.md) — Store, criação de `jrteste`, licenças, compras, evidências e teste físico;
- [Day 9](HACKATHON_DEVLOG/day9.md) — conquista do MVP e potencial de marketplace global;
- [JrSkill Execution API](JRSKILL_API.md) — formato, executor e histórico inicial; conferir a nota de atualização no documento;
- [App embarcado / Minhas Skills](JRSKILL_APP.md) — `JrBot_V1S_APP_03` e testes registrados;
- [Primeira Skill](JRSKILL_FIRST_SKILL.md) — guia para criadores;
- [Stage 2: Skill PDA](JRSKILL_SOLANA_STAGE2.md), [Stage 3](JRSKILL_SOLANA_STAGE3.md), [Stage 4](JRSKILL_SOLANA_STAGE4.md) — evolução das validações;
- [Modelo License PDA](JRSKILL_LICENSE_MODEL_00.md) e [Piloto](JRSKILL_PILOT.md) — regras e limitações do ciclo;
- [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33) — histórico e rastreabilidade.

## Documentos principais

### Produto e projeto

- [Roadmap oficial](ROADMAP.md)
- [Estado atual](PROJECT_STATUS.md)
- [Arquitetura](ARCHITECTURE.md)
- [Guia de desenvolvimento](DEVELOPMENT.md)
- [Changelog](CHANGELOG.md)
- [Testes e validação](TESTING.md)
- [Padrão da documentação](DOCUMENTATION.md)

### Interfaces e comportamento

- [Runtime API v1](API_RUNTIME.md)
- [Flow Engine](FLOWS.md)
- [Playground](PLAYGROUND.md)

### Hardware

- [Pinagem HW04](PINAGEM.md)
- [Esquema de ligação](ESQUEMA_LIGACAO.md)
- [Materiais](MATERIAIS.md)
- [Revisão HW04](REVISAO_HW04.md)

### Publicação

- [Apresentação pública](PUBLICACAO.md)

## Documentação antiga

Arquivos que descrevem branches, firmwares ou experimentos antigos foram movidos para `../trash/`.

Esse conteúdo foi preservado apenas para revisão histórica e **não deve ser usado como referência do estado atual**. O diretório será revisado manualmente antes de qualquer exclusão definitiva.

## Níveis de evidência

Cada documento deve distinguir:

- **Planejado** — arquitetura ou trabalho aprovado, ainda não implementado;
- **Implementado** — código existe;
- **Build verificado** — compilação concluída;
- **Bancada/simulado** — comportamento exercitado sem comprovar o efeito físico final;
- **Validado fisicamente** — comportamento confirmado no hardware real.

## Regra de versões

- `JrBot V1 / V2 / V3` = linhas de produto;
- `JrBot V1S` = linha oficial paralela JrSkill / Solana;
- `Runtime API 1.x` = versão técnica do protocolo;
- identificadores de build/revisão não substituem a versão de produto.
