# Guia de desenvolvimento

## Pré-requisitos

- ESP-IDF 5.5.x para a linha atual;
- Python disponível no ambiente;
- dependências do painel em `tools/jrbot_frontend/requirements.txt`;
- placa ESP32-S3 compatível com `JRBOT-HW-04` para validação física.

## Estratégia de branches

As branches oficiais são:

- `main`: última versão aprovada;
- `develop`: integração antes da promoção;
- `v1`: JrBot V1, linha ativa;
- `v2`: JrBot V2, linha preservada e pausada até fechar V1;
- `archive/*`: histórico.

Quando V3 começar, a linha será `v3`.

Não crie uma branch nova para cada revisão técnica de firmware. Revisões como `_V1_02`, `_V1_03` e `_V1_04` ficam na branch da versão e são diferenciadas por commit, marcador de firmware e evidência de teste.

A promoção segue:

```text
v1 -> develop -> main
v2 -> develop -> main
v3 -> develop -> main
```

## Fluxo recomendado

### 1. Escolha a versão

Para a etapa atual:

```bat
git switch v1
git pull --ff-only origin v1
```

Para reproduzir a integração:

```bat
git switch develop
git pull --ff-only origin develop
```

### 2. Verifique pinagem

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

### 3. Compile antes de gravar

```bat
INSTALAR.bat build
```

### 4. Grave

```bat
INSTALAR.bat flash
```

### 5. Abra o painel

```bat
PAINEL.bat
```

### 6. Confirme versão

Não interprete logs de uma candidata sem confirmar primeiro a versão retornada pela placa.

### 7. Registre a revisão

Toda revisão testável deve ter marcador inequívoco no firmware e no procedimento de teste. O commit e a versão técnica identificam a candidata; o nome da branch continua sendo a versão do produto.

### 8. Pull Request

O PR da versão para `develop` deve registrar:

- objetivo;
- contratos/API alterados;
- módulos afetados;
- build realizado;
- testes de computador;
- testes de bancada;
- validação física;
- limitações abertas;
- documentação atualizada.

## Estrutura principal

```text
firmware/
  core/                comandos/configuração básica
  main/                entrada e dependências do firmware
  module_audio/        MAX98357A e barramento compartilhado
  module_brain/        cérebro local, voz e respostas
  module_camera/       câmera/diagnóstico
  module_face/         OLED/expressões
  module_mic/          MS3625 e gravação
  module_portal/       portal HTTP do ESP32

tools/jrbot_frontend/  painel local no computador
hardware/              fonte de verdade da pinagem
docs/                  documentação oficial
tests/                 verificações automatizadas
```

Os nomes podem variar conforme a evolução; consulte o repositório antes de assumir um módulo.

## Como desenvolver uma capability nova

1. Confirme em qual versão do produto a capability pertence.
2. Defina o contrato público na documentação.
3. Identifique recursos físicos usados e conflitos.
4. Implemente na branch da versão atual.
5. Adicione validação de argumentos e observabilidade.
6. Compile.
7. Faça teste de bancada.
8. Faça validação física.
9. Atualize `PROJECT_STATUS.md` somente com o nível comprovado.
10. Registre o resultado no PR da versão para `develop`.

## Como alterar comportamento

Antes de modificar firmware, pergunte:

> A capability necessária já existe?

Se sim, no futuro a mudança deve preferencialmente virar configuração/Flow em runtime.

Exemplo:

```text
face + say + wait + listen
```

não deve exigir recompilação quando o Flow Engine estiver implementado.

## Logs

Logs devem registrar fatos úteis, não conclusões não comprovadas.

Bom:

```text
PLAY_RECORDING_END result=ESP_OK bytes=26880
```

Isso prova que o caminho de software terminou com sucesso. A documentação do teste ainda deve registrar separadamente se o áudio foi fisicamente ouvido.
