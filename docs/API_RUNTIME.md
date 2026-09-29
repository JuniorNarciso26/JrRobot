# JrBot Runtime API v1

**Candidata atual: `JRBotV2_RUNTIME_API_V1_02`.**

**Status: API base validada fisicamente pela Serial na candidata 01; extensão 1.1 com `face` implementada no código e ainda pendente de build/validação física.**

A Runtime API separa capacidades do firmware dos clientes que as utilizam. Painel, CLI, VPS e futuros aplicativos devem consumir o mesmo contrato em vez de criar comandos paralelos para cada interface.

## Versionamento

O campo do envelope permanece:

```json
{"v":1}
```

`v=1` representa o major compatível do contrato.

A candidata 02 publica:

```text
api = 1.1
```

A adição de `face` é compatível e, portanto, não cria um novo major `v=2`.

## Estado da candidata 02

Implementado no código:

- envelope JSON versionado `v=1`;
- `capabilities`;
- `get` para estados seguros;
- action `face`;
- erros JSON estruturados;
- transporte pela infraestrutura existente:
  - Serial: `api <JSON>`;
  - HTTP local: `POST /cmd` com corpo `api <JSON>`;
- descoberta dinâmica contendo apenas recursos realmente implementados.

Ainda não implementado:

- `set`;
- persistência de configuração;
- `say`, `wait`, `listen` e `play` como actions;
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

`id` é opcional e serve para correlação do cliente. Aceita letras, números, `_`, `-` e `.` com até 32 caracteres.

## Transporte Serial

```text
api {"v":1,"id":"c1","fn":"capabilities","args":{}}
```

Com o envelope correlacionado do terminal:

```text
@abc123 api {"v":1,"id":"c1","fn":"get","args":{"path":"system.version"}}
```

Resposta:

```text
JR_REPLY id=abc123 ok=1 JR_API {"v":1,"ok":true,"id":"c1","result":{...}}
```

A candidata 01 foi exercitada fisicamente pela Serial com `capabilities`, múltiplos `get` e erros estruturados.

## Transporte HTTP local

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

A validação HTTP continua sendo um item explícito do roteiro da candidata 02.

## `capabilities`

```json
{
  "v": 1,
  "fn": "capabilities",
  "args": {}
}
```

Na candidata 02 o resultado deve informar, entre outros campos:

```text
api: 1.1
functions: capabilities, get, face
actions: face
persistent_config: false
flow_engine: false
playground: false
```

`triggers` e `events` continuam vazios nesta etapa.

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

Caminhos implementados:

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

## `face`

`face` é a primeira action registrada da Runtime API.

Exemplo:

```json
{
  "v": 1,
  "id": "face01",
  "fn": "face",
  "args": {
    "expression": "thinking"
  }
}
```

Transporte em uma linha:

```text
api {"v":1,"id":"face01","fn":"face","args":{"expression":"thinking"}}
```

Resposta esperada:

```text
JR_API {"v":1,"ok":true,"id":"face01","result":{"expression":"thinking"}}
```

A action chama somente o módulo de face registrado no firmware. Ela não recebe GPIO, endereço de memória ou código arbitrário.

O teste físico deve confirmar duas coisas separadas:

1. o OLED realmente mudou de expressão;
2. `get("face.current")` devolve o mesmo estado após a action.

Expressões não reconhecidas devem retornar `invalid_args` com `face_expression_not_supported` e não devem alterar o estado.

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

```text
get("system.version")
set("audio.volume", 35)
face("happy")
say("oi")
wait(300)
listen()
```

No transporte essas operações continuam representadas por dados estruturados, nunca por execução de código enviado pelo cliente.

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

Mudanças incompatíveis exigem novo major do contrato. Adições compatíveis permanecem em `v=1` e devem aparecer dinamicamente em `capabilities()`.
