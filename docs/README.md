# Documentação do JrBot

Este diretório contém a documentação **ativa** do projeto. O estado vigente deve ser lido a partir destes documentos e do `README.md` da raiz.

## Estado atual

- release oficial: **`JrBot_V1.7.04`** em `main`;
- integração: `develop`, atualmente na mesma baseline da release;
- linha oficial paralela JrSkill/Solana: **`V1s-00`** / `JrBot_V1S_00`;
- linha de voz local: `v2`;
- referência histórica da V1: `v1`.

A documentação da `main` descreve apenas o **objetivo e o papel** da V1S. Andamento, etapas, provas e decisões internas da JrSkill Network permanecem documentados na própria branch `V1s-00`.

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
