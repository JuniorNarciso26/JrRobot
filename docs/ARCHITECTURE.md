# Arquitetura do JrBot

## Visão

O JrBot separa hardware, capacidades seguras e interfaces de controle.

```text
Interfaces / App / Painel / futuras IAs
                 ↓
        capacidades controladas
                 ↓
        modo / recursos físicos
                 ↓
             ESP32-S3
     OLED · câmera · áudio · mic
```

A regra central é simples: nenhuma camada externa deve obter acesso arbitrário a GPIO, memória ou drivers.

## Baseline estável — V1.7.04

### WebRTC local

A Live usa uma única sessão WebRTC para áudio bidirecional e JPEG da câmera:

```text
OV5640 -> JPEG -> DataChannel/SCTP -> navegador
MS3625 -> I2S RX -> G.711A/PCMA -> RTP/SRTP -> navegador
navegador -> RTP/SRTP -> G.711A/PCMA -> I2S TX -> MAX98357A
```

No HW04:

- BCLK GPIO21;
- WS/LRCLK GPIO47;
- RX SD GPIO41;
- TX DIN GPIO42.

RX e TX operam simultaneamente compartilhando BCLK/WS.

### Mode Manager

A V1.7.04 possui um `jr_mode_manager` mínimo para registrar o modo lógico do sistema.

Modos previstos:

```text
IDLE
AUTONOMOUS
LIVE
RECORDING
PLAYBACK
DIAGNOSTIC
```

Na baseline estável, o fluxo principal fisicamente consolidado é `IDLE -> LIVE -> IDLE`.

### Resource Manager

O `jr_resource_manager` é a autoridade central da trava do I2S compartilhado.

Owners definidos:

```text
NONE
MIC
PLAYBACK
LIVE
LEGACY_VOICE
```

O manager registra ownership e telemetria; os drivers continuam responsáveis pela configuração dos canais físicos.

## Runtime API

A Runtime API fornece um contrato estruturado para funções registradas do firmware.

Ela existe para que painel, ferramentas, futuras aplicações e outras camadas possam reutilizar as mesmas operações seguras em vez de criar acesso direto ao hardware.

Contrato vigente: [API_RUNTIME.md](API_RUNTIME.md).

## V1S — JrSkill Network

A V1S é uma linha oficial paralela.

Seu objetivo arquitetural é desacoplar **o comportamento desejado por uma Skill** da **forma como o hardware executa esse comportamento**:

```text
Skill declarativa
       ↓
Skill Executor seguro
       ↓
capability / Runtime API
       ↓
hardware JrBot
```

A camada baseada em Solana pode representar publicação, versão, distribuição e licença de Skills, mas não substitui o executor seguro local.

```text
Solana
  ↓
dados da Skill / licença
  ↓
validação local
  ↓
Skill Executor
  ↓
capabilities permitidas
  ↓
JrBot
```

A documentação da `main` não acompanha as etapas internas dessa pesquisa. Detalhes de implementação e evolução permanecem na branch `V1s-00`.

## V2 — voz local

A linha V2 estuda controle por voz local reutilizando as mesmas capacidades físicas. ESP-SR/MultiNet e seus conflitos específicos pertencem a essa linha.

## Princípios de segurança

1. conteúdo externo não escreve GPIO diretamente;
2. funções disponíveis precisam estar registradas/allowlisted;
3. argumentos são validados antes da execução;
4. recursos físicos compartilhados precisam de arbitragem explícita;
5. estado e versão devem ser observáveis por log/status;
6. configurações inválidas não podem impedir recuperação básica do robô;
7. exposição remota futura exige autenticação e arquitetura própria.

## Evolução

A arquitetura deve evoluir de forma aditiva. Protocolos e módulos já validados não devem ser reescritos apenas para acomodar uma nova linha experimental.
