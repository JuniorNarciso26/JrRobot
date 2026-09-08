# JrBot Runtime API v1

**Status da candidata `JRBotV2_RUNTIME_API_V1_01`: BASE IMPLEMENTADA, ainda sem build/validação física registrados.**

A Runtime API separa capacidades do firmware dos clientes que as utilizam. Painel, CLI, VPS e futuros aplicativos devem consumir o mesmo contrato em vez de criar comandos paralelos para cada interface.

## Estado desta candidata

Implementado no código:

- envelope JSON versionado `v=1`;
- função `capabilities`;
- função `get` para estados seguros;
- erros JSON estruturados;
- transporte pela infraestrutura de comandos já existente:
  - Serial: `api <JSON>`;
  - HTTP local: `POST /cmd` com corpo `api <JSON>`;
- lista de capabilities contendo somente recursos realmente implementados.

Ainda não implementado:

- `set`;
- persistência de configuração;
- `say`, `face`, `wait`, `listen` como actions da Runtime API;
- Voice Registry dinâmico;
- Playground;
- Flow Engine;
- eventos assíncronos da Runtime API;
- WebSocket;
- autenticação/TLS para uso fora de rede local confiável.

## Envelope da API

Requisição:

```json
{
  "v": 1,
  "id": "teste01",
  "fn": "capabilities",
  "args": {}
}
```

Resposta de sucesso:

```text
JR_API {"v":1,"ok":true,"id":"teste01","result":{...}}
```

Resposta de erro:

```text
JR_API {"v":1,"ok":false,"id":"teste01","error":{"code":"not_found","message":"function_not_supported"}}
```

`id` é opcional e serve para correlação do cliente. Nesta versão aceita apenas letras, números, `_`, `-` e `.` com até 32 caracteres.

## Transporte Serial

O contrato é transportado pelo comando textual legado apenas como envelope de transporte:

```text
api {"v":1,"id":"c1","fn":"capabilities","args":{}}
```

Com o envelope correlacionado já existente no terminal:

```text
@abc123 api {"v":1,"id":"c1","fn":"get","args":{"path":"system.version"}}
```

O terminal responde primeiro com o protocolo de correlação Serial e, dentro dele, a resposta da Runtime API:

```text
JR_REPLY id=abc123 ok=1 JR_API {"v":1,"ok":true,"id":"c1","result":{...}}
```

## Transporte HTTP local

A candidata reutiliza o endpoint local existente:

```text
POST /cmd
X-JrBot-Command: 1
Content-Type: text/plain
```

Corpo:

```text
api {"v":1,"fn":"capabilities","args":{}}
```

O portal atual continua destinado apenas a rede local confiável; não possui autenticação/TLS para exposição pública.

## `capabilities`

A descoberta é a primeira função da API.

```json
{
  "v": 1,
  "fn": "capabilities",
  "args": {}
}
```

O resultado informa:

- versão da API;
- firmware;
- hardware e profile;
- funções disponíveis;
- caminhos suportados por `get`;
- transportes disponíveis;
- flags explícitas indicando que persistência, Flow Engine e Playground ainda estão desativados.

As listas `actions`, `triggers` e `events` ficam vazias nesta candidata. Isso é intencional: `capabilities()` não anuncia funcionalidades planejadas como se estivessem disponíveis.

## `get`

Formato:

```json
{
  "v": 1,
  "id": "estado01",
  "fn": "get",
  "args": {
    "path": "brain.status"
  }
}
```

Caminhos implementados na candidata 01:

```text
api.version
system.version
system.hardware
system.profile
brain.status
voice.status
audio.volume
face.current
wifi.status
```

Exemplo:

```text
api {"v":1,"fn":"get","args":{"path":"audio.volume"}}
```

Resposta conceitual:

```text
JR_API {"v":1,"ok":true,"result":{"path":"audio.volume","value":35}}
```

## Erros estruturados

Códigos iniciais:

| Código | Significado |
|---|---|
| `invalid_json` | corpo não é um objeto JSON válido |
| `invalid_request` | envelope inválido |
| `invalid_args` | argumentos incompatíveis com a função |
| `unsupported_version` | versão diferente de `v=1` |
| `not_found` | função ou caminho não disponível |
| `no_memory` | falha de alocação/serialização |
| `response_too_large` | resposta não cabe no buffer do protocolo |

O firmware não executa código textual arbitrário. `fn` precisa corresponder a uma função registrada internamente.

## Direção futura

A sintaxe conceitual continuará simples:

```text
get("system.version")
set("audio.volume", 35)
face("happy")
say("oi")
wait(300)
listen()
```

No transporte, porém, essas operações devem continuar representadas por dados estruturados, por exemplo:

```json
{
  "v": 1,
  "fn": "face",
  "args": {
    "expression": "happy"
  }
}
```

## Voice API planejada

```text
voice.add(phrase)
voice.remove(id)
voice.list()
voice.get(id)
```

A confiança deve ser associada ao trigger/regra configurável sempre que possível, em vez de depender somente de um threshold global do modelo.

## Playground planejado

```text
playground.start(phrase, interval_ms)
playground.stop()
playground.save(confidence)
```

Durante calibração, o robô deverá coletar candidatos sem executar o Flow normal.

## Flow API planejada

```text
flow.save(...)
flow.list()
flow.get(id)
flow.delete(id)
flow.enable(id)
flow.disable(id)
flow.run(id)
```

## Compatibilidade

Toda capability futura deve documentar:

- versão mínima da API;
- argumentos;
- retorno;
- erros;
- persistência;
- recursos físicos utilizados;
- eventos gerados;
- nível de validação.

Mudanças incompatíveis exigem nova versão do contrato. Adições compatíveis permanecem em `v=1` e devem aparecer dinamicamente em `capabilities()`.
