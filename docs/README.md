# Documentação do JrBot

Este diretório é a **fonte oficial de documentação do projeto**. O navegador de documentação do painel local lê estes mesmos arquivos Markdown.

## Como ler a documentação

Cada documento deve distinguir claramente:

- **Implementado** — código existe no repositório.
- **Build verificado** — compilação/empacotamento foi verificado.
- **Bancada/simulado** — comportamento foi exercitado sem provar o fenômeno físico final.
- **Validado fisicamente** — houve teste real no hardware.
- **Planejado** — arquitetura aprovada para desenvolvimento, mas ainda não implementada.

Nunca transforme um item planejado em “disponível” apenas porque existe uma especificação.

## Guia rápido

### Projeto

- [Estado atual](PROJECT_STATUS.md)
- [Arquitetura](ARCHITECTURE.md)
- [Guia de desenvolvimento](DEVELOPMENT.md)
- [Testes e validação](TESTING.md)
- [Padrão da documentação](DOCUMENTATION.md)

### Plataforma configurável

- [Runtime API v1](API_RUNTIME.md)
- [Roteiro de teste Runtime API v1 — candidata 01](RUNTIME_API_V1_TEST.md)
- [Roteiro de teste Runtime API v1 — candidata 02](RUNTIME_API_V1_02_TEST.md)
- [Playground](PLAYGROUND.md)
- [Flows](FLOWS.md)

### Hardware e histórico técnico existente

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

Esses documentos históricos registram etapas específicas e podem citar versões anteriores. Para saber o estado vigente, consulte primeiro [PROJECT_STATUS.md](PROJECT_STATUS.md).

## Estrutura desejada a longo prazo

```text
docs/
├── README.md
├── PROJECT_STATUS.md
├── ARCHITECTURE.md
├── DEVELOPMENT.md
├── TESTING.md
├── DOCUMENTATION.md
├── API_RUNTIME.md
├── RUNTIME_API_V1_TEST.md
├── RUNTIME_API_V1_02_TEST.md
├── PLAYGROUND.md
├── FLOWS.md
└── documentos técnicos/históricos existentes
```

À medida que o projeto crescer, os documentos podem ser migrados para subdiretórios por domínio sem quebrar a regra principal: uma única fonte de verdade em `docs/`.
