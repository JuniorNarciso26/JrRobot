# Padrão de documentação

## Objetivo

A documentação deve permitir que outra pessoa compreenda o estado do JrBot sem depender do histórico de conversa do mantenedor.

## Fonte de verdade

Documentos do projeto ficam em `docs/`.

O painel local apenas renderiza esses arquivos; ele não mantém cópias próprias do conteúdo.

## Todo recurso deve declarar status

Use uma das classificações:

```text
PLANEJADO
IMPLEMENTADO
BUILD VERIFICADO
BANCADA/SIMULADO
VALIDADO FISICAMENTE
```

Um documento pode conter partes em estados diferentes.

## Documentar uma capability

Inclua:

1. propósito;
2. nome público;
3. argumentos;
4. valores permitidos;
5. retorno;
6. erros;
7. recursos físicos usados;
8. concorrência;
9. persistência;
10. eventos/logs;
11. segurança;
12. procedimento de teste;
13. estado atual.

## Documentar uma API

Separe:

- contrato lógico;
- transporte;
- autenticação/segurança;
- versionamento;
- exemplos;
- status de implementação.

Não misture sintaxe amigável de Flow com execução de código arbitrário.

## Documentar hardware

Atualize em conjunto:

- `hardware/pinmap.json`;
- `docs/PINAGEM.md`;
- esquema de ligação relevante;
- testes/políticas de GPIO.

## ADRs futuros

Decisões arquiteturais importantes podem migrar para `docs/decisions/` usando registros curtos:

```text
contexto
opções consideradas
decisão
consequências
```

Exemplos de decisões que merecem ADR:

- formato de armazenamento dos Flows;
- transporte oficial da Runtime API;
- autenticação do portal;
- mecanismo de wake word definitivo;
- estratégia de TTS.

## Idiomas

A documentação principal está em português nesta fase. Quando houver demanda internacional, a tradução deve manter uma versão canônica identificada e evitar divergência silenciosa entre idiomas.
