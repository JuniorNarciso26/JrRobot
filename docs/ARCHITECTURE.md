# Arquitetura do JrBot

## Visão

O JrBot segue uma arquitetura híbrida:

```text
ESP32-S3
  corpo + segurança + funções locais
        |
        | API/eventos
        v
Painel / App / VPS
        |
        v
IA avançada opcional
```

O robô deve manter identidade e funções básicas offline. Serviços remotos entram para linguagem livre, memória avançada, visão avançada, TTS sofisticado e outras capacidades que não precisam residir no microcontrolador.

## Camadas

### 1. Hardware

Drivers e arbitragem de recursos:

- microfone;
- áudio;
- display;
- câmera;
- futuros atuadores e sensores.

### 2. Firmware capabilities

O firmware expõe operações seguras. Exemplos conceituais:

```text
face("happy")
say("oi")
listen()
camera("capture")
```

Essas funções não são equivalentes a permitir código arbitrário. Cada função deve mapear para uma implementação limitada e validada no firmware.

### 3. Voice Engine

Responsável por:

- registrar frases suportadas pelo mecanismo local;
- receber candidatos do reconhecedor;
- medir confiança;
- aplicar regras por frase;
- publicar eventos de detecção/calibração.

O MultiNet6 atual é uma solução experimental para reconhecimento de comandos. Um wake word dedicado `JrBot` continua sendo uma opção arquitetural futura.

### 4. Flow Engine — planejado

Transforma eventos em sequências de ações seguras:

```text
trigger -> conditions -> actions
```

Exemplo:

```text
voice("JR BOT")
confidence >= 0.58
-> face("happy")
-> say("oi")
-> listen()
```

### 5. Storage — planejado

Armazena configuração de runtime, como:

- frases;
- limites de confiança;
- Flows;
- preferências;
- referências a assets de áudio.

Configuração pequena pode usar armazenamento chave/valor; dados maiores, como WAVs, devem usar armazenamento de arquivos apropriado. A escolha final ainda deve ser formalizada antes da implementação.

### 6. Runtime API — planejada

Contrato independente do painel. O painel deve ser apenas um cliente dessa API.

Isso permite reutilização por:

- painel local;
- CLI;
- aplicativo;
- VPS;
- IA;
- automações.

## Live local WebRTC — V1.6.3

A Live local da V1.6.3 usa uma única PeerConnection para áudio bidirecional e transporte do JPEG da câmera.

```text
OV5640 -> JPEG -> DataChannel/SCTP -> navegador
MS3625 -> I2S RX -> G.711A/PCMA -> RTP/SRTP -> navegador
navegador -> RTP/SRTP -> G.711A/PCMA -> I2S TX -> MAX98357A
```

No HW04, RX e TX I2S operam simultaneamente compartilhando:

- BCLK GPIO21;
- WS/LRCLK GPIO47;
- RX SD GPIO41;
- TX DIN GPIO42.

O HTTPS local serve página e signaling. O vídeo da Live não usa mais uma requisição HTTPS por quadro.

A sessão usa ICE local, DTLS-SRTP e SCTP. O caminho foi validado fisicamente com áudio nas duas direções e vídeo contínuo. A arquitetura atual é local; evolução para Internet deverá preservar WebRTC e acrescentar sinalização remota, STUN/TURN ou infraestrutura equivalente.

O cancelamento de eco acústico não faz parte da V1.6.3 e permanece como pesquisa futura.

## Princípios de segurança

1. IA não escreve GPIO diretamente.
2. Todo atuador é mediado por uma capability do firmware.
3. Argumentos são validados antes da execução.
4. Configurações persistentes devem ser versionadas e validadas.
5. Uma configuração inválida não pode impedir boot básico do robô.
6. Deve existir fallback/factory config para recuperação.
7. Upload de arquivos e configuração pelo portal exigem limites de tamanho e validação.

## Compatibilidade

O protocolo textual atual (`status`, `brain_status`, `audio_test`, etc.) não precisa desaparecer imediatamente. A Runtime API poderá ser adicionada acima dele ou ao lado dele e migrar funcionalidades progressivamente.

O objetivo é evolução aditiva, não uma reescrita total.
