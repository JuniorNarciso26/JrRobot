# JrBot Flow Engine

**Status: PLANEJADO / especificação inicial.**

Flows descrevem comportamento sem recompilar firmware.

## Modelo

```text
TRIGGER
   ↓
CONDITIONS
   ↓
ACTIONS
```

Exemplo:

```text
voice("JR BOT")
confidence >= 0.58
-> face("happy")
-> say("oi")
-> wait(300)
-> listen()
```

## Representação estruturada

```json
{
  "schema": 1,
  "id": "wake_jrbot",
  "enabled": true,
  "trigger": {
    "fn": "voice",
    "args": {
      "phrase": "JR BOT",
      "confidence": 0.58
    }
  },
  "actions": [
    {"fn": "face", "args": {"expression": "happy"}},
    {"fn": "say", "args": {"content": "oi"}},
    {"fn": "wait", "args": {"ms": 300}},
    {"fn": "listen", "args": {}}
  ]
}
```

## Por que JSON/estrutura e não código arbitrário

O firmware deve executar apenas funções registradas e validadas.

Nunca:

```text
receber texto -> interpretar C/Python -> escrever GPIO
```

Sempre:

```text
Flow -> action registry -> validação -> capability segura
```

## Action Registry

Cada função precisa de metadados.

Exemplo conceitual:

```json
{
  "name": "face",
  "version": 1,
  "args": {
    "expression": "enum"
  },
  "resource": "oled"
}
```

Isso permite `capabilities()` e validação antes de salvar um Flow.

## Ações iniciais propostas

- `face`
- `say`
- `play`
- `wait`
- `listen`
- `stopListen`
- `camera.capture`

Ações de movimento só entram quando drivers/limites físicos correspondentes estiverem implementados e validados.

## Triggers iniciais propostos

- `voice`
- futuro: timer local;
- futuro: evento de sensor;
- futuro: evento seguro recebido da API/VPS.

## Execução

O Flow Engine não deve bloquear tarefas críticas. `wait(5000)` significa agendar continuação do Flow, não travar processamento essencial por cinco segundos.

## Concorrência

Antes da implementação devem ser definidos:

- um Flow por vez ou múltiplos;
- prioridade;
- cancelamento;
- timeout;
- disputa por áudio/câmera/motores;
- comportamento quando uma action falha.

Para a primeira versão, a opção mais previsível é execução serial de um Flow local por vez.

## Erros

Um Flow deve produzir eventos estruturados:

```text
flow.started
flow.finished
flow.error
action.started
action.finished
action.error
```

Falha de uma action precisa indicar se o Flow:

- aborta;
- continua;
- executa fallback.

Essa política deve ser explícita no schema, não implícita no código.

## Versionamento

Todo Flow persistido deve possuir `schema`.

Firmware novo que não consiga interpretar um schema antigo deve recusar o Flow com erro claro e preservar a configuração para recuperação, em vez de executar comportamento parcialmente interpretado.
