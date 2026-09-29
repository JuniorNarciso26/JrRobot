# Teste da JrBot Runtime API v1

Candidata: `JRBotV2_RUNTIME_API_V1_01`

Status inicial deste documento: **roteiro preparado; resultados físicos ainda não registrados.**

## 1. Build

No terminal ESP-IDF 5.5.x:

```bat
git switch feature/runtime-api-v1
git pull --ff-only origin feature/runtime-api-v1
INSTALAR.bat build
```

Build isolado esperado:

```text
firmware/build-runtime-api-v1-01
firmware/sdkconfig.runtime-api-v1-01
```

Não avance para gravação se o build falhar.

## 2. Gravação

```bat
INSTALAR.bat flash
```

Abra o painel:

```bat
PAINEL.bat
```

Confirme primeiro:

```text
JRBotV2_RUNTIME_API_V1_01
```

## 3. `capabilities` pela Serial

No campo de comando manual do painel, envie em uma única linha:

```text
api {"v":1,"id":"cap01","fn":"capabilities","args":{}}
```

Esperado:

```text
JR_API {"v":1,"ok":true,"id":"cap01","result":{...}}
```

O `result` deve anunciar somente:

```text
functions: capabilities, get
persistent_config: false
flow_engine: false
playground: false
actions: []
triggers: []
events: []
```

## 4. `get` básicos

Teste individualmente:

```text
api {"v":1,"id":"v1","fn":"get","args":{"path":"api.version"}}
api {"v":1,"id":"v2","fn":"get","args":{"path":"system.version"}}
api {"v":1,"id":"v3","fn":"get","args":{"path":"system.hardware"}}
api {"v":1,"id":"v4","fn":"get","args":{"path":"system.profile"}}
api {"v":1,"id":"v5","fn":"get","args":{"path":"audio.volume"}}
api {"v":1,"id":"v6","fn":"get","args":{"path":"face.current"}}
api {"v":1,"id":"v7","fn":"get","args":{"path":"brain.status"}}
api {"v":1,"id":"v8","fn":"get","args":{"path":"voice.status"}}
api {"v":1,"id":"v9","fn":"get","args":{"path":"wifi.status"}}
```

Cada chamada deve retornar `ok:true`, o mesmo `id` e o `path` solicitado.

## 5. Erros estruturados

### Versão incompatível

```text
api {"v":2,"id":"err1","fn":"capabilities","args":{}}
```

Esperado:

```text
"ok":false
"code":"unsupported_version"
```

### Função inexistente

```text
api {"v":1,"id":"err2","fn":"nao.existe","args":{}}
```

Esperado:

```text
"ok":false
"code":"not_found"
"message":"function_not_supported"
```

### Caminho inexistente

```text
api {"v":1,"id":"err3","fn":"get","args":{"path":"system.nao_existe"}}
```

Esperado:

```text
"ok":false
"code":"not_found"
"message":"get_path_not_supported"
```

### JSON inválido

```text
api {isso-nao-e-json}
```

Esperado:

```text
"ok":false
"code":"invalid_json"
```

## 6. Teste HTTP local

Com o Wi-Fi do JrBot ativo e a partir de uma máquina na mesma rede local, envie um `POST /cmd` com:

```text
X-JrBot-Command: 1
Content-Type: text/plain
```

Corpo:

```text
api {"v":1,"id":"http01","fn":"get","args":{"path":"system.version"}}
```

A resposta deve começar com:

```text
JR_API {"v":1,"ok":true,"id":"http01"
```

O portal atual não possui autenticação/TLS e não deve ser exposto diretamente à Internet.

## 7. Regressão

Depois dos testes da API, confirme que os comandos legados continuam funcionando:

```text
status
brain_status
version
```

Também confirme, conforme o hardware disponível, que painel, OLED, áudio, microfone e câmera não sofreram regressão.

Nesta etapa não é necessário recalibrar o reconhecimento de `JrBot`; a candidata foi desenhada para preservar a lógica de voz da revisão anterior.

## Critério para aprovar a base

A base Runtime API v1 pode ser integrada em `develop` quando:

- build concluir sem erro;
- `capabilities` funcionar;
- todos os `get` implementados responderem corretamente;
- erros inválidos retornarem JSON estruturado;
- Serial continuar correlacionando respostas;
- HTTP local funcionar;
- não houver regressão evidente nos comandos legados.

Validação física adicional deve ser registrada separadamente de build e teste de protocolo.
