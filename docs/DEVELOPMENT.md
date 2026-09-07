# Guia de desenvolvimento

## Pré-requisitos

- ESP-IDF 5.5.x para a linha atual;
- Python disponível no ambiente;
- dependências do painel em `tools/jrbot_frontend/requirements.txt`;
- placa ESP32-S3 compatível com `JRBOT-HW-04` para validação física.

## Fluxo recomendado

### 1. Escolha a branch

Para a etapa atual de resposta/voz:

```bat
git switch feature/local-brain-jrbot-response
git pull --ff-only origin feature/local-brain-jrbot-response
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

## Regra de versão

Mudanças físicas/testáveis importantes devem possuir marcador inequívoco no firmware e no procedimento de teste.

Exemplo atual:

```text
JRBotV2_JRBOT_RESPONSE_05
```

Evite reutilizar diretórios de build/config quando a investigação depende de eliminar configuração residual.

## Como desenvolver uma capability nova

1. Defina o contrato público na documentação.
2. Identifique recursos físicos usados e conflitos.
3. Implemente a função interna com validação de argumentos.
4. Adicione observabilidade/logs.
5. Adicione comando/API de teste.
6. Compile.
7. Faça teste de bancada.
8. Faça validação física.
9. Atualize `PROJECT_STATUS.md` somente com o nível comprovado.
10. Depois permita que o Flow Engine use essa capability.

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
