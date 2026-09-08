# Teste da JrBot Runtime API v1 — candidata 02

Candidata: `JRBotV2_RUNTIME_API_V1_02`

Branch de origem: `feature/runtime-api-v1-02`

Status: **resultado físico registrado em 2026-09-08; aprovada para integração com ressalvas documentadas.**

Esta revisão mantém o protocolo JSON principal em `v=1`, publica a API compatível como `1.1`, adiciona a primeira action `face` e altera o framing de saída do MAX98357A de MSB para I2S Philips.

## 1. Build e execução

Build isolado configurado:

```text
firmware/build-runtime-api-v1-02
firmware/sdkconfig.runtime-api-v1-02
```

A placa executou e reportou:

```text
JRBotV2_RUNTIME_API_V1_02
```

O log completo da etapa de compilação ESP-IDF não foi anexado ao registro final. Portanto, execução/flash físico e evidência de build permanecem estados documentados separadamente.

## 2. Áudio MAX98357A

A saída foi alterada de:

```text
I2S_STD_MSB_SLOT_DEFAULT_CONFIG
```

para:

```text
I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG
```

O teste registrou:

```text
format=PHILIPS
TEST_END ... result=ESP_OK
```

### Resultado físico

- o ruído forte da candidata anterior desapareceu;
- o som foi avaliado como bom/limpo;
- o controle de volume de 5 a 100 continuou funcional;
- o volume acústico máximo permaneceu baixo;
- a limitação de volume foi aceita nesta etapa como característica do alto-falante de 20 mm / 4 ohms / 3 W e montagem acústica atual.

Medições de alimentação registradas durante a investigação:

```text
repouso: ~4,99 V
queda momentânea durante áudio: ~4,56 V
durante reprodução: ~4,88 V
```

Ligar `GAIN` ao GND não produziu mudança acústica relevante. Melhoria de alto-falante/caixa fica fora do escopo desta revisão.

## 3. `capabilities`

Comando:

```text
api {"v":1,"id":"cap02","fn":"capabilities","args":{}}
```

Resultado registrado:

```text
"ok":true
"api":"1.1"
```

As funções anunciadas incluem:

```text
capabilities
get
face
```

E `actions` contém:

```text
face
```

Playground, Flow Engine e persistência continuam indisponíveis nesta candidata.

## 4. Action `face` pela Serial

Comando:

```text
api {"v":1,"id":"face01","fn":"face","args":{"expression":"thinking"}}
```

Resultado:

```text
JR_API {"v":1,"ok":true,"id":"face01","result":{"expression":"thinking"}}
```

O OLED mudou fisicamente para `thinking`.

A leitura subsequente:

```text
api {"v":1,"id":"face02","fn":"get","args":{"path":"face.current"}}
```

confirmou:

```text
"value":"thinking"
```

A cadeia validada foi:

```text
Runtime API -> action registrada -> módulo de face -> OLED físico -> leitura do estado real
```

## 5. Expressão inválida

Teste:

```text
api {"v":1,"id":"faceerr1","fn":"face","args":{"expression":"nao_existe"}}
```

Resultado:

```text
"ok":false
"code":"invalid_args"
"message":"face_expression_not_supported"
```

A action não aceita uma expressão arbitrária fora do conjunto suportado pelo firmware.

## 6. Regressão básica da API v1

Foram repetidos com sucesso na candidata 02:

```text
get("system.version")
get("audio.volume")
get("brain.status")
```

`audio.volume` também refletiu a alteração real de `35` para `100`, mostrando leitura do estado efetivo do firmware.

### Não repetido especificamente na candidata 02

Os testes abaixo haviam sido validados na candidata 01, mas não foram repetidos nos logs finais da candidata 02 antes da integração:

```text
v=2 -> unsupported_version
JSON inválido -> invalid_json
```

Essa lacuna permanece explícita e não é tratada como nova validação física da candidata 02.

## 7. HTTP local

Foi validado `POST /cmd` pela rede local com:

```text
X-JrBot-Command: 1
Content-Type: text/plain
```

Consulta de versão:

```text
api {"v":1,"id":"http01","fn":"get","args":{"path":"system.version"}}
```

retornou `JRBotV2_RUNTIME_API_V1_02`.

Action física pela rede:

```text
api {"v":1,"id":"httpface","fn":"face","args":{"expression":"happy"}}
```

retornou:

```text
JR_API {"v":1,"ok":true,"id":"httpface","result":{"expression":"happy"}}
```

O OLED mudou fisicamente para `happy`.

Assim foi validada a cadeia:

```text
PC -> Wi-Fi -> HTTP /cmd -> Runtime API -> face() -> OLED físico
```

O portal local continua sem autenticação/TLS e não deve ser exposto diretamente à Internet.

## 8. Resultado da candidata 02

### Validado fisicamente / em protocolo

- execução do firmware `JRBotV2_RUNTIME_API_V1_02`;
- I2S Philips no MAX98357A;
- áudio sem o ruído forte anterior;
- `capabilities` API `1.1`;
- action `face` pela Serial;
- confirmação por `get("face.current")`;
- rejeição de expressão inválida;
- regressão de leituras principais da candidata 01;
- HTTP local para `get`;
- HTTP local para `face` com alteração física do OLED.

### Limitações conhecidas

- volume acústico baixo com o alto-falante atual;
- diagnóstico do microfone ainda pode confundir I2S ocupado com `unavailable`;
- `unsupported_version` e `invalid_json` não foram repetidos especificamente na candidata 02;
- Playground, Flow Engine e persistência ainda não existem.

Resultado de build, resultado de protocolo e validação física continuam sendo registrados separadamente.