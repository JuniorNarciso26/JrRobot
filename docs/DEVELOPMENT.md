# Guia de desenvolvimento

## Pré-requisitos

- ESP-IDF 5.5.x para a linha atual;
- Python disponível no ambiente;
- dependências do painel em `tools/jrbot_frontend/requirements.txt`;
- hardware compatível com `JRBOT-HW-04` para validação física.

## Branches oficiais

| Branch | Papel |
| --- | --- |
| `main` | release oficial estável — `JrBot_V1.7.04` |
| `develop` | integração antes da próxima promoção |
| `V1s-00` | linha oficial paralela JrSkill Network / Solana |
| `v2` | linha de controle por voz local |
| `v1` | referência histórica da V1 |

A `V1s-00` não é uma branch temporária de feature. Ela é uma linha oficial paralela do projeto. Sua eventual integração em `develop/main` depende de decisão técnica explícita.

Detalhes de desenvolvimento, provas e etapas da V1S permanecem na própria branch.

## Fluxo normal de desenvolvimento

Para trabalho que parte da baseline oficial:

```text
IDEIA
→ PESQUISA
→ ISSUE
→ ARQUITETURA
→ BRANCH ISOLADA
→ PROVA
→ BUILD
→ TESTE FÍSICO
→ DOCUMENTAÇÃO
→ develop
→ main
```

Não misture experimentos diferentes na mesma branch.

Branches `test/*` são laboratório e não devem ser promovidas diretamente para `develop`. Quando uma prova funcionar, a solução deve ser limpa e integrada por uma branch adequada.

## Trabalhar sobre a baseline atual

```bash
git switch develop
git pull --ff-only origin develop
```

Nova feature:

```bash
git switch -c feature/<nome>
```

Correção:

```bash
git switch -c hotfix/<nome>
```

## Trabalhar na linha JrSkill / Solana

Use a branch oficial:

```bash
git switch V1s-00
git pull --ff-only origin V1s-00
```

Não copie automaticamente mudanças experimentais da V1S para `main` ou `develop`.

A documentação detalhada dessa linha deve permanecer dentro da própria branch até existir decisão de integração.

## Trabalhar na linha V2

```bash
git switch v2
git pull --ff-only origin v2
```

A V2 é a linha de voz local e deve permanecer separada de V1S.

## Verificação de hardware

Antes de alterar pinagem:

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

A fonte de verdade de pinagem continua em `hardware/pinmap.json`.

## Build

```bat
INSTALAR.bat build
```

Build bem-sucedida comprova **compilação**, não comportamento físico.

## Flash

```bat
INSTALAR.bat flash
```

Depois do flash, confirme no log/status a identificação exata do firmware.

## Teste físico

O teste físico deve registrar:

- build/versão instalada;
- cenário;
- comportamento observado;
- logs relevantes;
- resultado;
- limitação conhecida.

Nunca transformar `ESP_OK`, resposta HTTP, frame JPEG ou mensagem de log em confirmação física sem observação real do hardware.

## Pull Requests

Todo PR deve informar:

- objetivo;
- Issue relacionada quando aplicável;
- branch/base;
- módulos afetados;
- impacto em hardware/recursos;
- estado de build;
- testes realizados;
- validação física;
- limitações;
- documentação atualizada.

## Documentação

Ao integrar trabalho relevante, revisar conforme o escopo:

- `README.md`;
- `docs/ROADMAP.md`;
- `docs/PROJECT_STATUS.md`;
- `docs/ARCHITECTURE.md`;
- `docs/CHANGELOG.md`;
- documento técnico afetado.

Documentos que não representam mais o estado vigente devem ser movidos para `trash/` antes de uma possível exclusão.

## Estrutura principal

```text
firmware/
  core/
  main/
  module_audio/
  module_brain/
  module_camera/
  module_face/
  module_portal/
  module_runtime/

tools/jrbot_frontend/
hardware/
docs/
tests/
trash/              histórico aguardando revisão
```
