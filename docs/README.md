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

### Produto e projeto

- [Roadmap oficial](ROADMAP.md)
- [Estado atual](PROJECT_STATUS.md)
- [Arquitetura](ARCHITECTURE.md)
- [Guia de desenvolvimento](DEVELOPMENT.md)
- [Testes e validação](TESTING.md)
- [Padrão da documentação](DOCUMENTATION.md)

### Plataforma configurável

- [Runtime API v1](API_RUNTIME.md)
- [Teste Runtime API candidata 01](RUNTIME_API_V1_TEST.md)
- [Teste Runtime API candidata 02](RUNTIME_API_V1_02_TEST.md)
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

- [Local Brain Test](LOCAL_BRAIN_TEST.md)
- [Voice Wake Test](VOICE_WAKE_TEST.md)
- [JrBot Response Test](JRBOT_RESPONSE_TEST.md)

Para saber o estado vigente, consulte primeiro [ROADMAP.md](ROADMAP.md) e [PROJECT_STATUS.md](PROJECT_STATUS.md).

## Regra de versões

- `JrBot V1 / V2 / V3` = versão do produto.
- `Runtime API 1.x` = versão técnica do protocolo.
- `_V1_02`, `_V1_03`, `_V1_04` etc. = candidata ou revisão técnica de firmware.

Essas numerações são independentes.
