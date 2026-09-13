# Roadmap oficial do JrBot

Atualizado em 2026-09-13.

## Nomenclatura

- `JrBot V0 / V1 / V2 / V3`: versões do produto.
- `Runtime API 1.1 / 1.2 / 1.3`: versões do protocolo e das capabilities.
- `JRBot..._V1_02 / _V1_03 / _V1_04`: candidatas ou revisões técnicas de firmware.

Essas numerações são independentes.

## Direção do produto

```text
JrBot V0  -> fundação de hardware, periféricos e painel de desenvolvimento
JrBot V1  -> painel interno pelo IP
JrBot V2  -> comandos por voz
JrBot V3  -> controle programático por API
JrBrain   -> memória, personalidade, LLM e comportamento
```

## JrBot V0 — concluída

Objetivo: criar e validar a fundação técnica do robô antes das versões de produto voltadas à interação normal do usuário.

A V0 consolidou:

- ESP32-S3 N16R8;
- OLED SSD1306 e expressões;
- áudio MAX98357A;
- microfone MS3625;
- câmera OV5640;
- Wi-Fi e portal local;
- painel de desenvolvimento e diagnóstico;
- testes dos principais periféricos;
- base inicial da Runtime API e capabilities seguras.

A V0 é a versão atualmente aprovada em `main`.

## JrBot V1 — em desenvolvimento

Objetivo: permitir que uma pessoa controle o JrBot pelo navegador na rede local, usando o painel servido pelo próprio ESP32-S3.

Escopo:

- visualizar estado do robô;
- mudar a expressão facial;
- ajustar volume e testar áudio;
- gravar áudio pelo microfone do JrBot;
- ouvir e baixar a gravação;
- reproduzir a gravação no JrBot;
- interface de câmera;
- envio de áudio do celular/computador quando o formato suportado estiver definido;
- interface responsiva e amigável para celular.

Linha atual: `feature/v1-internal-web-panel`, PR #18 para `develop`.

O `PAINEL.bat` e o painel Python são ferramentas de instalação, desenvolvimento e diagnóstico. O painel interno do ESP32 é a interface homem-robô da V1.

## JrBot V2

Objetivo: acionar as mesmas capabilities do robô por voz.

Exemplos:

```text
JrBot feliz
JrBot tocar som abc.wav
```

O trabalho de Playground, threshold, calibração e latência de voz pertence a esta versão. O PR #13 é a linha consolidada dessa frente.

## JrBot V3

Objetivo: permitir que software externo controle as mesmas capabilities sem depender do painel ou da voz.

A V3 é definida pelo tipo de integração, não por um transporte específico. A Runtime API pode usar HTTP/IP, Serial ou outro transporte compatível.

A existência da Runtime API atual não significa que a JrBot V3 esteja concluída como produto.

## JrBrain

Depois da fundação V1-V3, o projeto avança para memória, identidade, personalidade, contexto, LLM, relacionamento e skills. Esse sistema deve continuar usando apenas as capabilities seguras disponibilizadas pelo firmware.

## Visão de longo prazo

```text
JrBrain
   ↓
Skills carregáveis
   ↓
Acessórios inteligentes
   ↓
Plataforma / ecossistema
```

Skills, acessórios e marketplace representam a visão futura do projeto e não devem ser confundidos com funcionalidades já concluídas.

## Promoção

```text
feature/* ou fix/* -> develop -> main
```

`main` representa a última versão de produto aprovada. `develop` integra a próxima versão. Implementação, build, teste de bancada e validação física devem continuar documentados separadamente.
