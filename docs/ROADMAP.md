# Roadmap oficial do JrBot

Atualizado em 2026-10-10 para documentar o marco JrSkill MVP.

## Baseline oficial

A referência estável fisicamente validada para controle local é **`JrBot_V1.7.04`**. O MVP JrSkill usa o build experimental `JrBot_V1S_APP_03` e comprova um fluxo Devnet distinto de uma nova release Mainnet.

`develop` parte da mesma baseline e recebe somente trabalho validado que esteja sendo preparado para futura promoção.

## Branches oficiais abertas

| Branch | Papel |
| --- | --- |
| `main` | release oficial estável — `JrBot_V1.7.04` |
| `develop` | integração de entregas validadas |
| `V1s-00` | MVP JrSkill / Solana concluído; histórico do Colosseum preservado |
| `v2` | linha preservada para controle por voz local |
| `v1` | referência histórica da linha V1 |

Branches temporárias de pesquisa, feature, fix, hotfix ou teste não são linhas oficiais de produto.

## Direção do projeto

```text
JrBot V1  -> controle local pelo navegador, câmera, áudio e WebRTC
JrBot V2  -> controle por voz local
JrBot V3  -> controle programático / evolução da Runtime API
JrBrain   -> memória, personalidade, contexto e inteligência

V1S / JrSkill Network
     -> linha paralela para Skills portáveis
     -> distribuição/versionamento/licenciamento via Solana
     -> execução física somente por capabilities seguras
```

## JrBot V1 — concluída

Objetivo: permitir o controle local do JrBot pelo navegador, com mídia e interação física no HW04.

Versão oficial: **`JrBot_V1.7.04`**.

A baseline consolidada inclui:

- ESP32-S3 N16R8;
- OLED SSD1306;
- câmera OV5640;
- microfone MS3625;
- amplificador MAX98357A;
- Wi-Fi e HTTPS local;
- foto e Live;
- WebRTC local com áudio full-duplex;
- JPEG por DataChannel;
- I2S RX/TX simultâneo com BCLK/WS compartilhados;
- `jr_mode_manager`;
- `jr_resource_manager` para ownership do I2S.

O histórico detalhado de releases permanece em [CHANGELOG.md](CHANGELOG.md).

## V1S — JrSkill Network

A **V1S** é a linha JrSkill Network. Seu **MVP do Colosseum foi concluído em 2026-10-10**, com testes em Solana Devnet e execução física validada no JrBot.

Objetivo: permitir que capacidades de IA física sejam descritas como Skills portáveis e declarativas, distribuídas/versionadas/licenciadas por uma camada baseada em **Solana**, sem permitir que conteúdo externo controle diretamente GPIO, drivers ou código nativo do ESP32.

Arquitetura conceitual:

```text
Skill
  ↓
Skill Executor seguro
  ↓
capabilities / Runtime API
  ↓
JrBot físico

Solana
  ↓
publicação / versão / licença / distribuição
```

O andamento, as etapas, as provas e a arquitetura experimental detalhada foram preservados em [HACKATHON_DEVLOG.md](HACKATHON_DEVLOG.md), com Kickoff e Day 1–9, e na branch de origem `V1s-00`. O projeto poderá prosseguir numa **nova linha definida separadamente**. O marketplace global continua uma visão futura, não uma operação Mainnet já validada.

## JrBot V2 — voz local

Objetivo: reutilizar as capabilities físicas do JrBot através de comandos por voz local.

A branch `v2` permanece preservada como linha própria. Questões específicas de ESP-SR/MultiNet e coexistência com outros recursos pertencem a essa frente.

## JrBot V3 — controle programático

Objetivo: evoluir o controle estruturado do robô por API, reutilizando as mesmas capabilities utilizadas pelas interfaces locais.

A Runtime API existente é uma base técnica; a V3 de produto deve ser definida separadamente quando a linha for oficialmente iniciada.

## JrBrain

Objetivo futuro: memória, personalidade, contexto, LLM, comportamento e integração de Skills.

## Regra de integração

Uma linha paralela não é promovida automaticamente para `develop` ou `main`.

Antes de integrar:

```text
Issue
→ pesquisa/arquitetura
→ branch isolada
→ implementação
→ build
→ teste físico
→ documentação
→ decisão de integração
→ develop
→ main
```
