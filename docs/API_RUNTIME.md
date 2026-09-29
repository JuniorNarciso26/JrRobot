# JrBot Runtime API v1

## Estado vigente

Contrato atual na `main`:

```text
major protocol: 1
implementation: 1.1
```

A Runtime API separa clientes e interfaces das implementações físicas do firmware.

Ela aceita somente funções registradas; não executa texto arbitrário, código nativo ou acesso direto a GPIO.

## Funções implementadas

A baseline atual registra:

- `capabilities` — consulta;
- `get` — consulta;
- `face` — action.

A descoberta dinâmica também publica os transports disponíveis.

## Envelope

Requisição:

```json
{
  "v": 1,
  "id": "req01",
  "fn": "capabilities",
  "args": {}
}
```

Resposta:

```text
JR_API {"v":1,"ok":true,"id":"req01","result":{...}}
```

Erros retornam o mesmo envelope com `ok=false`.

O campo `id` é opcional e serve para correlação do cliente.

## Transports

### Serial

```text
api {"v":1,"fn":"capabilities","args":{}}
```

### HTTP local

```text
POST /cmd
X-JrBot-Command: 1
Content-Type: text/plain
```

Corpo:

```text
api {"v":1,"fn":"capabilities","args":{}}
```

O HTTP atual é destinado à rede local confiável. `X-JrBot-Command: 1` não é autenticação para exposição pública.

## capabilities

A resposta inclui:

- versão da API;
- protocolo;
- firmware;
- hardware;
- profile;
- funções;
- paths de `get`;
- actions;
- transports;
- flags de features ainda não disponíveis.

Na baseline atual:

```text
functions = capabilities, get, face
actions   = face
persistent_config = false
flow_engine       = false
playground        = false
```

## get

Paths registrados:

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
wifi.diagnostics
```

Exemplo:

```json
{
  "v": 1,
  "fn": "get",
  "args": {
    "path": "system.version"
  }
}
```

## face

A action `face` recebe uma expressão suportada pelo renderer:

```json
{
  "v": 1,
  "fn": "face",
  "args": {
    "expression": "happy"
  }
}
```

A função chama somente o módulo de face registrado no firmware.

Expressões desconhecidas retornam erro estruturado e não autorizam acesso direto ao display ou GPIO.

## Erros estruturados

Códigos usados pelo contrato incluem:

| Código | Significado |
| --- | --- |
| `invalid_json` | corpo JSON inválido |
| `invalid_request` | envelope inválido |
| `invalid_args` | argumentos incompatíveis |
| `unsupported_version` | major não suportado |
| `not_found` | função/path não disponível |
| `no_memory` | falha de alocação/serialização |
| `response_too_large` | resposta excede o buffer |

## Segurança

A Runtime API aplica uma allowlist de funções.

Não é permitido através desse contrato:

- execução arbitrária de C/C++;
- shell;
- acesso bruto a GPIO;
- acesso bruto à memória;
- nomes de função não registrados.

Esse limite também é a base para futuras camadas, incluindo voz, JrBrain e JrSkill Network.

## Evolução

Novas capabilities compatíveis podem continuar em `v=1` e devem aparecer em `capabilities()`.

Mudanças incompatíveis exigem novo major do contrato.

Detalhes de candidatas e roteiros antigos de teste foram movidos para `trash/`; eles não representam o contrato vigente.
