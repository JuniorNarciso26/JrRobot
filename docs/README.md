# Documentação do JrBot

Este diretório é a fonte oficial de documentação do projeto. O navegador de documentação do painel local lê estes mesmos arquivos Markdown.

## Como ler a documentação

Cada documento deve distinguir claramente:

- **Implementado** — código existe no repositório.
- **Build verificado** — compilação foi verificada.
- **Bancada/simulado** — comportamento foi exercitado sem provar o efeito físico final.
- **Validado fisicamente** — houve teste real no hardware.
- **Planejado** — aprovado para desenvolvimento, mas ainda não implementado.

## Guia rápido

### JrSkill Network / Hackathon 2026

- [JrSkill Execution API — documento atual para Colosseum](JRSKILL_API.md)
- [Issue #33 — histórico técnico do JrSkill Network](https://github.com/JuniorNarciso26/JrRobot/issues/33)

Para o trabalho atual de JrSkill, **JRSKILL_API.md é a referência principal**. Ele separa claramente o que já foi validado fisicamente do que ainda está planejado.

### Produto e projeto

- [Roadmap oficial](ROADMAP.md)
- [Estado atual](PROJECT_STATUS.md)
- [Arquitetura](ARCHITECTURE.md)
- [Guia de desenvolvimento](DEVELOPMENT.md)
- [Testes e validação](TESTING.md)
- [Padrão da documentação](DOCUMENTATION.md)

### Plataforma configurável

- [Runtime API v1 — contrato de baixo nível e histórico](API_RUNTIME.md)
- [Playground](PLAYGROUND.md)
- [Flows](FLOWS.md)

### Hardware e histórico técnico

- [Pinagem](PINAGEM.md)
- [Esquema de ligação](ESQUEMA_LIGACAO.md)
- [Materiais](MATERIAIS.md)
- [Plataforma](PLATAFORMA.md)
- [Faces V2](V2-FACES.md)
- [Revisão HW04](REVISAO_HW04.md)

### Testes históricos

Estes documentos são preservados como evidência técnica de etapas anteriores. Eles não substituem a documentação vigente do JrSkill.

- [Teste Runtime API candidata 01](RUNTIME_API_V1_TEST.md)
- [Teste Runtime API candidata 02](RUNTIME_API_V1_02_TEST.md)
- [Local Brain Test](LOCAL_BRAIN_TEST.md)
- [Voice Wake Test](VOICE_WAKE_TEST.md)
- [JrBot Response Test](JRBOT_RESPONSE_TEST.md)

Para o estado geral vigente, consulte [ROADMAP.md](ROADMAP.md) e [PROJECT_STATUS.md](PROJECT_STATUS.md). Para a arquitetura do hackathon, consulte [JRSKILL_API.md](JRSKILL_API.md).

## Regra de versões

- `JrBot V1 / V2 / V3` = versão do produto.
- `JrBot V1S` = linha experimental JrSkill / Solana.
- `Runtime API 1.x` = versão técnica do protocolo.
- `_V1_02`, `_V1_03`, `_V1_04` etc. = candidata ou revisão técnica histórica de firmware.

Essas numerações são independentes.
