# JrBot - robo com IA

JrBot e um projeto de robo com inteligencia artificial em desenvolvimento, baseado em **ESP32-S3**, com painel de controle no computador, rosto OLED opcional, camera e audio modular.

**A IA e parte do objetivo do produto, nao uma funcionalidade ja validada nesta versao.** O codigo atual concentra-se em comunicacao, diagnostico e testes isolados de hardware. Microfone, conversacao por IA e movimentos ainda precisam de implementacao/validacao.

## Comece pela branch correta

A implementacao atual esta em [`v2-revisada`](https://github.com/JuniorNarciso26/JrRobot/tree/v2-revisada). O codigo de firmware em `main` e uma base historica; esta atualizacao da pagina publica nao o transforma na V2. **Nao compile `main` para testar esta revisao.**

A atualizacao eletrica e **JRBOT-HW-03**, identificada no firmware como **JRBOT-V2-DIAG-02**. O painel continua sendo a interface de operacao; nao e preciso abrir monitor Serial separado.

## Restricao de hardware importante

**GPIO21, GPIO41, GPIO42 e GPIO47 ja estao ocupados e nao podem receber novas ligacoes.** A atribuicao anterior de audio foi revogada. Audio sai sem GPIOs atribuidos e precisa de nova confirmacao fisica. OLED continua reservado em **SDA=GPIO1 / SCL=GPIO2**, mas desabilitado no diagnostico atual.

Nao desconecte os circuitos ja ligados aos quatro GPIOs ocupados. Ainda precisamos identificar sua funcao e o fabricante/revisao da placa para fechar a montagem fisica.

## Documentacao

| Documento | Conteudo |
|---|---|
| [Materiais](docs/MATERIAIS.md) | Quantidades, componentes, acessorios e itens ainda a identificar |
| [Pinagem](docs/PINAGEM.md) | Inventario de todos os grupos de GPIOs, bloqueios e recursos internos |
| [Esquema de ligacao](docs/ESQUEMA_LIGACAO.md) | Sinais, alimentacao, USB, camera, OLED e pendencias de audio/microfone |
| [Operar pelo painel](PAINEL_V2.md) | COM4, botoes, versao e diagnostico sem monitor |
| [Atualizar o firmware](APLICAR_BRANCH_V2.md) | Branch, compilacao e gravacao na COM6 |
| [Arquitetura e IA](docs/PLATAFORMA.md) | Implementado, planejado e limites |
| [Publicacao](docs/PUBLICACAO.md) | Descricao do GitHub, privacidade, licenca e estado publico |

## Estado real

| Parte | Estado |
|---|---|
| Painel + protocolo Serial | Firmware DIAG-01 respondeu na montagem do proprietario; HW03 ainda requer novo build/gravacao |
| OLED | Opcional; sua ausencia nao bloqueia o robo. Driver SSD1306, sem suporte SH1106 declarado |
| Amplificador MAX98357A | Teste I2S sob demanda; pinagem antiga revogada, nova atribuicao pendente |
| Camera | Teste de um quadro JPEG, opt-in; nao e streaming nem foco automatico |
| Microfone | Modelo/interface/pinos nao confirmados; sem driver de entrada |
| Servo e motores | Nao implementados; nenhum GPIO atribuido |
| IA / STT / TTS | Planejados; nenhum servico de IA integrado nesta candidata |

## Fluxo de trabalho

Use a pasta do projeto fora da instalacao do ESP-IDF. No terminal ESP-IDF 5.5.x, com alteracoes locais revisadas:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
DIAG_V2.bat build
DIAG_V2.bat flash
PAINEL.bat
```

Execute uma linha de cada vez e pare em qualquer erro. `flash` compila e grava na **COM6**; o painel opera pela **COM4**, conforme esta montagem. O Windows pode renumerar portas. Nao use dois programas na mesma COM.

No painel, selecione Serial USB, COM4 e Conectar. Atualizar o painel nao grava a placa. Testes nao executam automaticamente ao conectar ou ligar.

## Contribuicao e seguranca

Leia [CONTRIBUTING.md](CONTRIBUTING.md). Nao publique credenciais, audio privado ou imagens sem autorizacao. Nao grave eFuses nem force resets do Git para fazer testes. O projeto e publico, mas a escolha de uma licenca para o codigo proprio ainda depende do proprietario; avisos de terceiros devem ser preservados.

Projeto criado por Junior. Identidade atual: **JrBot**. O nome tecnico do repositorio permanece `JuniorNarciso26/JrRobot` para nao quebrar clones e links existentes.
