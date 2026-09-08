# Teste da JrBot Runtime API v1 — candidata 02

Candidata: `JRBotV2_RUNTIME_API_V1_02`

Branch: `feature/runtime-api-v1-02`

Status inicial: **implementado no código; build e validação física ainda pendentes.**

Esta revisão mantém o protocolo JSON principal em `v=1`, publica a API compatível como `1.1`, adiciona a primeira action `face` e altera o framing de saída do MAX98357A de MSB para I2S Philips.

## 1. Atualizar a branch

```bat
git fetch --all --prune
git switch feature/runtime-api-v1-02
git pull --ff-only origin feature/runtime-api-v1-02
```

## 2. Build

No terminal ESP-IDF 5.5.x:

```bat
INSTALAR.bat build
```

Build isolado esperado:

```text
firmware/build-runtime-api-v1-02
firmware/sdkconfig.runtime-api-v1-02
```

Não avance para o flash se o build falhar.

## 3. Gravar e confirmar versão

```bat
INSTALAR.bat flash
PAINEL.bat
```

Confirmar:

```text
JRBotV2_RUNTIME_API_V1_02
```

## 4. Teste do áudio MAX98357A

Desative o modo autônomo antes do teste manual de áudio.

Aplique volume baixo/moderado, por exemplo 15% ou 20%, e execute `audio_test`.

No log do firmware deve aparecer:

```text
format=PHILIPS
```

O critério físico é simples: o tom deve ser contínuo e reconhecível, sem o ruído forte observado na candidata anterior.

Se o log indicar `ESP_OK` mas o som continuar sendo apenas ruído, registrar o resultado como **falha física** e investigar alimentação, GND, SD/MODE, ligação BCLK/WS/DIN e alto-falante. Não aumentar o volume para mascarar a falha.

Depois do tom, se houver uma gravação válida de microfone disponível, testar também a reprodução da gravação para separar qualidade do framing de saída de eventual clipping da captura.

## 5. `capabilities`

Enviar:

```text
api {"v":1,"id":"cap02","fn":"capabilities","args":{}}
```

Esperado:

```text
"ok":true
"api":"1.1"
```

As funções devem incluir:

```text
capabilities
get
face
```

E `actions` deve conter:

```text
face
```

Playground, Flow Engine e persistência continuam `false`/indisponíveis nesta candidata.

## 6. Primeira action real: `face`

Enviar:

```text
api {"v":1,"id":"face01","fn":"face","args":{"expression":"thinking"}}
```

Esperado:

```text
JR_API {"v":1,"ok":true,"id":"face01","result":{"expression":"thinking"}}
```

### Validação física

O OLED deve mudar realmente para a expressão `thinking`.

Em seguida consultar o mesmo estado pela API:

```text
api {"v":1,"id":"face02","fn":"get","args":{"path":"face.current"}}
```

Esperado:

```text
"ok":true
"value":"thinking"
```

Esse teste valida a cadeia:

```text
Runtime API -> action registrada -> módulo de face -> OLED -> leitura do estado real
```

Repetir com outra expressão conhecida, por exemplo:

```text
api {"v":1,"id":"face03","fn":"face","args":{"expression":"happy"}}
```

## 7. Expressão inválida

Enviar:

```text
api {"v":1,"id":"face_err","fn":"face","args":{"expression":"nao_existe"}}
```

Esperado:

```text
"ok":false
"code":"invalid_args"
"message":"face_expression_not_supported"
```

O OLED não deve mudar por causa dessa chamada inválida.

## 8. Regressão da API v1

Repetir pelo menos:

```text
api {"v":1,"id":"v1","fn":"get","args":{"path":"system.version"}}
api {"v":1,"id":"v2","fn":"get","args":{"path":"audio.volume"}}
api {"v":1,"id":"v3","fn":"get","args":{"path":"brain.status"}}
api {"v":2,"id":"err1","fn":"capabilities","args":{}}
api {isso-nao-e-json}
```

A semântica validada na candidata 01 deve permanecer intacta.

## 9. HTTP local

Com o JrBot e o computador na mesma rede, testar `POST /cmd` com o cabeçalho:

```text
X-JrBot-Command: 1
Content-Type: text/plain
```

Corpo sugerido:

```text
api {"v":1,"id":"http_face","fn":"face","args":{"expression":"thinking"}}
```

A resposta deve ser `ok:true` e o OLED deve mudar fisicamente. O portal local continua sem autenticação/TLS e não deve ser exposto à Internet.

## 10. Critério de aprovação

A candidata 02 pode retornar para `develop` quando:

- build concluir sem erro;
- firmware reportar `JRBotV2_RUNTIME_API_V1_02`;
- `audio_test` registrar `format=PHILIPS`;
- a qualidade física do tom for avaliada;
- `capabilities` anunciar API `1.1` e action `face`;
- `face("thinking")` mudar fisicamente o OLED;
- `get("face.current")` confirmar o mesmo estado;
- expressão inválida for rejeitada de forma estruturada;
- regressões básicas da candidata 01 não aparecerem;
- HTTP local for testado ou permanecer explicitamente marcado como pendente.

Resultado de build, resultado de protocolo e qualidade física de áudio devem ser registrados separadamente.
