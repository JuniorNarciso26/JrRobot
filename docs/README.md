# Documentação do JrBot

Este diretório segue a mesma organização da documentação ativa da `main` e adiciona os documentos exclusivos da linha **JrSkill Network / V1s-00**.

Assim, a branch V1s permanece atualizada com a baseline documental do projeto sem perder o histórico, as provas e as decisões específicas do hackathon.

## Estado atual

- release oficial: **`JrBot_V1.7.04`** em `main`;
- integração: `develop`;
- linha oficial paralela JrSkill/Solana: **`V1s-00`** / `JrBot_V1S_00`;
- linha de voz local: `v2`;
- referência histórica da V1: `v1`.

A documentação geral abaixo acompanha a organização da `main`. O progresso da V1s fica registrado somente nesta branch.

## JrSkill Network / V1s-00

Documentos exclusivos desta linha:

- [JrSkill Execution API](JRSKILL_API.md) — contrato atual do Skill JSON, Executor e Recipes;
- [Etapa 2 — Solana Skill PDA](JRSKILL_SOLANA_STAGE2.md) — arquitetura, publicação/leitura do JSON v1 e plano de teste isolado;
- [Hackathon Development Log](HACKATHON_DEVLOG.md) — índice do diário, com Kickoff e Day 1–3 separados em [HACKATHON_DEVLOG/](HACKATHON_DEVLOG/);
- [Issue #33 — JrSkill Network / Solana Hackathon 2026](https://github.com/JuniorNarciso26/JrRobot/issues/33) — histórico técnico e decisões.

Esses documentos devem ser preservados durante toda a V1s. Quando a linha for aprovada para integração, eles fornecem o histórico necessário para decidir o que deve subir para a documentação oficial.

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

Esse conteúdo foi preservado apenas para revisão histórica e **não deve ser usado como referência do estado atual**.

Os documentos específicos da V1s — `JRSKILL_API.md`, o índice `HACKATHON_DEVLOG.md` e os registros diários em `HACKATHON_DEVLOG/` — permanecem ativos em `docs/`.

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
