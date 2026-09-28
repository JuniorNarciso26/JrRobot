# JrBot V1

<p align="center">
  <img src="docs/assets/readme/jrbot-hero.svg" alt="JrBot - presença digital" width="100%">
</p>

**Uma IA que deixa de ser apenas uma ferramenta e passa a ser presença.**

O **JrBot V1** é a primeira versão comercial do projeto JrBot: um robô físico, conectado à rede local, com expressão digital, câmera, microfone, alto-falante e uma interface própria acessível pelo celular ou computador.

A proposta do JrBot é criar uma experiência de presença: você vê, escuta, fala, observa o ambiente e controla o robô por uma interface simples, enquanto a plataforma continua evoluindo em direção a voz local, memória, personalidade e Skills.

> **O JrBot não foi criado apenas para responder. Foi criado para estar com você.**

<p align="center">
  <strong>Open-source ESP32-S3 robot with voice, camera, WebRTC, expressive OLED face and an evolving AI architecture.</strong>
</p>

## Contribua com o JrBot

O **JrBot é open source** e recebe contribuições de pessoas interessadas em robótica, sistemas embarcados, áudio, visão, interfaces e inteligência artificial.

Você não precisa dominar todo o projeto nem possuir um JrBot completo para começar. Há contribuições que podem ser desenvolvidas e revisadas de forma isolada, como documentação, testes, interface web, ferramentas e melhorias específicas de firmware.

### Áreas em que você pode ajudar

- **ESP32-S3 / ESP-IDF** — firmware, drivers, memória, Wi-Fi e arquitetura embarcada;
- **Áudio e DSP** — I2S, full-duplex, eco/AEC, codecs e processamento de voz;
- **WebRTC e redes** — comunicação em tempo real, sinalização e evolução para acesso remoto;
- **Frontend** — painel web responsivo, experiência no celular e ferramentas de diagnóstico;
- **Câmera e visão** — OV5640, captura, streaming e futuras capacidades de visão computacional;
- **Eletrônica e hardware** — alimentação, áudio, sensores, placas e revisão de integração;
- **Design mecânico / impressão 3D** — corpo, suportes, encaixes e evolução física do robô;
- **Testes e documentação** — testes automatizados, documentação técnica, guias e exemplos;
- **IA / JrBrain** — arquitetura futura de memória, personalidade, contexto, LLM e Skills.

### Como começar

1. Leia o [guia de contribuição](CONTRIBUTING.md) e a [documentação técnica](docs/README.md).
2. Consulte as [Issues abertas](https://github.com/JuniorNarciso26/JrRobot/issues) e escolha um problema ou melhoria com escopo claro.
3. Comente na Issue antes de iniciar uma implementação maior, para alinhar abordagem e evitar trabalho duplicado.
4. Trabalhe em uma mudança pequena e isolada, preferencialmente em um fork ou branch dedicado.
5. Abra um Pull Request descrevendo claramente o que foi implementado, o que foi compilado/testado e o que ainda depende de validação física.

O projeto procura manter um histórico técnico compreensível:

```text
Issue
  ↓
arquitetura / pesquisa
  ↓
branch
  ↓
implementação
  ↓
teste
  ↓
Pull Request
  ↓
decisão
```

Contribuições pequenas são bem-vindas. Uma correção de documentação, um teste, um diagrama, uma melhoria de interface ou uma prova isolada podem ser tão úteis quanto uma alteração grande no firmware.


## JrBot V1 — o que já está disponível

Versão oficial atual: **`JrBot_V1.7.04`**

A V1 transforma a base eletrônica do JrBot em um produto utilizável diretamente pelo navegador.

### Novidades da V1.7.04

A `JrBot_V1.7.04` fecha a revisão de performance da V1 e consolida a baseline local validada no HW04.

Principais melhorias acumuladas:

- Live WebRTC local com áudio bidirecional PCMA/G.711A em 8 kHz e vídeo JPEG da OV5640 via DataChannel/SCTP;
- I2S full-duplex real no ESP32-S3 com BCLK GPIO21 e WS GPIO47 compartilhados entre RX e TX;
- redução de interferência do App, removendo o polling automático recorrente de `/status`;
- primeiro `jr_mode_manager` central, com o fluxo físico validado `idle -> live -> idle`;
- primeiro `jr_resource_manager` para o I2S compartilhado;
- ownership explícito do I2S para `mic`, `playback` e `live`;
- telemetria de aquisições, liberações, busy e liberações inconsistentes do I2S;
- gravação pelo MS3625 e reprodução pelo MAX98357A preservadas;
- câmera OV5640, foto e Live preservadas com orientação física de -90°/270°;
- política de memória/PSRAM preservada para manter ICE/DTLS/SCTP, câmera e áudio operando em conjunto.

Na validação física final, microfone, playback e Live adquiriram e liberaram o I2S pelo gerenciador central, com `i2s_release_mismatch=0`, retorno do owner para `none`, WebRTC conectado e encerrado normalmente e sem reset/watchdog observado.

A Live continua local. A evolução para Internet, STUN/TURN e sinalização remota permanece como etapa futura. O reconhecimento local por ESP-SR/MultiNet pertence à V2 de voz e não faz parte da baseline oficial desta V1.7.04.

### Controle pelo celular ou computador

O JrBot possui um painel web servido pelo próprio ESP32-S3. Na mesma rede local, o usuário pode acessar o robô pelo navegador sem depender do painel Python de desenvolvimento.

A interface foi preparada para **PC e celular** e permite acompanhar o estado do robô e executar suas principais funções.

### Expressões no rosto

O rosto do JrBot usa um display OLED SSD1306 e pode exibir diferentes expressões.

Pelo painel é possível:

- selecionar expressões;
- alterar o rosto do JrBot em tempo real;
- visualizar qual expressão está ativa;
- usar as expressões como base para futuras respostas emocionais e comportamentais.

### Câmera OV5640

A V1 integra a câmera ao painel do JrBot.

Recursos disponíveis:

- captura de fotos;
- visualização da imagem no navegador;
- modo de câmera ao vivo;
- resoluções QVGA, VGA e SVGA;
- ajuste de qualidade JPEG;
- métricas de FPS e tamanho dos quadros;
- orientação física confirmada da câmera em -90 graus / 270 graus;
- geração do JPEG respeitando a orientação definida no firmware.

### Microfone do JrBot

O microfone MS3625 pode ser utilizado diretamente pela interface.

A V1 permite:

- gravar áudio pelo próprio JrBot;
- escolher a duração da gravação dentro dos limites definidos;
- ouvir a gravação no navegador;
- baixar o arquivo WAV;
- reproduzir a última gravação pelo alto-falante do próprio JrBot;
- acompanhar informações de estado do microfone.

### Enviar áudio para o JrBot

O usuário também pode enviar mensagens de áudio do celular ou computador para o robô.

Isso permite usar o JrBot como um ponto físico de reprodução de mensagens, aproveitando o amplificador MAX98357A e o alto-falante integrado.

### Live WebRTC full-duplex

O fluxo principal de comunicação da V1.7.04 é uma Live local bidirecional.

Ao iniciar a Live:

- o microfone MS3625 envia áudio para o navegador por WebRTC;
- o microfone do celular envia áudio para o MAX98357A por WebRTC;
- a câmera OV5640 envia JPEGs pelo DataChannel da mesma sessão;
- os clocks I2S BCLK e WS permanecem compartilhados entre RX e TX;
- o usuário pode mutar cada direção de áudio sem encerrar a sessão.

O PTT legado deixa de ser o fluxo principal da interface. O transporte atual foi validado na rede local e foi desenhado para permitir evolução futura da arquitetura WebRTC.

### Áudio e volume

Pelo painel é possível:

- ajustar o volume;
- executar teste de som;
- reproduzir gravações;
- reproduzir mensagens recebidas;
- acompanhar o estado do sistema de áudio.

A arquitetura utiliza o MAX98357A para saída e o `jr_resource_manager` para arbitrar o I2S compartilhado entre microfone, playback e Live.

### Wi-Fi e acesso local

O JrBot V1 foi projetado para entrar na rede de forma simples.

Recursos:

- DHCP como comportamento padrão;
- IP atribuído automaticamente pelo roteador;
- exibição do IP atual no painel;
- estado da conexão Wi-Fi;
- configuração e diagnóstico de rede;
- leitura dos parâmetros persistidos;
- reconexão automática prevista pela camada de Wi-Fi.

O produto não depende de um IP fixo configurado manualmente para funcionar.

### HTTPS local

A V1 inclui HTTPS local para liberar recursos seguros dos navegadores modernos.

Isso é especialmente importante no celular para funções como:

- acesso ao microfone;
- `getUserMedia`;
- `MediaRecorder`;
- gravação de voz pelo navegador.

O HTTPS local foi incorporado para permitir uma experiência mais completa sem depender de serviços externos para o controle básico do robô.

## Hardware da V1

A plataforma atual utiliza:

- **ESP32-S3 N16R8** — processamento principal;
- **OLED SSD1306** — rosto e expressões;
- **OV5640** — câmera;
- **MS3625** — microfone;
- **MAX98357A** — amplificador de áudio;
- **alto-falante integrado**;
- Wi-Fi para comunicação local.

## Como a V1 funciona

```text
Celular / computador
        ↓
      Wi-Fi
        ↓
 painel web local
        ↓
    ESP32-S3
   ↙   ↓    ↘
OLED câmera  áudio
      ↓       ↓
 microfone  alto-falante
```

O painel usa operações controladas do firmware. A capability layer unificada continua planejada; a experiência normal não expõe GPIO bruto nem execução arbitrária.

## O que diferencia a V1

A V1 reúne em um único robô físico:

**Ver** — câmera com foto e transmissão ao vivo.  
**Ouvir** — microfone integrado e gravação.  
**Falar** — mensagens de áudio e PTT pelo alto-falante.  
**Expressar** — rosto digital com diferentes expressões.  
**Conectar** — controle direto pelo navegador na rede local.

Tudo isso forma a base sobre a qual serão adicionadas as próximas camadas de inteligência do JrBot.

## Estado do produto

| Versão | Objetivo | Estado |
| --- | --- | --- |
| **V0** | Fundação de hardware e periféricos | Concluída |
| **V1** | Controle humano pelo navegador e mídia local | **Concluída — `JrBot_V1.7.04`** |
| **V2** | Controle por voz local | Próxima etapa |
| **V3** | Controle programático pela Runtime API | Planejada |
| **JrBrain** | Memória, personalidade, contexto, LLM e Skills | Futuro |

A `JrBot_V1.7.04` está promovida em `main` como versão oficial atual. A `develop` parte desta mesma baseline para as próximas entregas validadas.

## Próximas evoluções

A evolução planejada preserva as capacidades construídas na V1:

```text
V1 — navegador e controle local
 ↓
V2 — voz local
 ↓
V3 — controle programático
 ↓
JrBrain — memória + personalidade + LLM
 ↓
Skills / plataforma / ecossistema
```

A V2 deverá reutilizar as mesmas capacidades físicas já consolidadas na V1, mas acionadas por voz.

O JrBrain será a camada futura de memória, personalidade, contexto e inteligência. Skills e experiências especializadas pertencem à visão de longo prazo e **não são funcionalidades da V1 atual**.

## Desenvolvimento e documentação

Branches principais:

```text
main    = última versão oficialmente promovida
develop = integração das próximas entregas
v1      = referência fechada da JrBot V1
v2      = desenvolvimento da JrBot V2
```

Documentação técnica:

- [Roadmap](docs/ROADMAP.md)
- [Estado do projeto](docs/PROJECT_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Desenvolvimento](docs/DEVELOPMENT.md)
- [Runtime API](docs/API_RUNTIME.md)
- [Testes](docs/TESTING.md)

## Licença

O conteúdo original deste repositório é disponibilizado sob a [MIT License](LICENSE), salvo indicação explícita em contrário. Isso permite uso, modificação, distribuição e uso comercial, preservados os avisos exigidos pela licença.

Contribuições aceitas são disponibilizadas sob a mesma licença MIT do projeto.

Dependências e componentes de terceiros continuam sujeitos às respectivas licenças.

A licença do código não concede automaticamente direito de apresentar produtos derivados como oficiais nem direitos sobre a marca **JrBot**.

## Visão

O JrBot começa como um robô controlável, expressivo e conectado.

A visão é evoluir para uma presença digital física capaz de ouvir, conversar, perceber o ambiente, manter continuidade e receber novas habilidades sem perder a simplicidade de interação.

<p align="center">
  <strong>JrBot</strong><br>
  Ouvir. Entender. Responder. Reagir. Expressar.<br><br>
  <em>Project by JrTk Invest</em>
</p>
