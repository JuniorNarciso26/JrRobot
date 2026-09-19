# Roadmap oficial do JrBot

Atualizado em 2026-09-19.

## Nomenclatura

- `JrBot V0 / V1 / V2 / V3`: versões do produto.
- `Runtime API 1.1 / 1.2 / 1.3`: versões do protocolo e das capabilities.
- `JRBot..._V1_02 / _V1_03 / _V1_04`: candidatas ou revisões técnicas de firmware.

Essas numerações são independentes.

## Branches oficiais

```text
main    -> última versão aprovada; atualmente V0
develop -> integração; atualmente contém a V1 final
v1      -> linha fechada da JrBot V1
v2      -> JrBot V2, linha preservada e pausada até estabilizar/promover V1
archive/* -> histórico
```

Novas melhorias ou correções da V1 devem partir de `develop` em branches `feature/*` ou `fix/*`. A branch `v1` passa a funcionar como referência fechada da versão entregue.

## Direção do produto

```text
JrBot V0  -> fundação de hardware, periféricos e painel de desenvolvimento
JrBot V1  -> controle humano pelo navegador na rede local
JrBot V2  -> comandos por voz local
JrBot V3  -> controle programático por API
JrBrain   -> memória, personalidade, LLM e comportamento
```

## JrBot V0 — concluída e aprovada

Objetivo: criar e validar a fundação técnica do robô antes das versões de produto voltadas à interação normal do usuário.

A V0 consolidou ESP32-S3 N16R8, OLED SSD1306, áudio MAX98357A, microfone MS3625, câmera OV5640, Wi-Fi, portal local, painel de desenvolvimento e a base inicial da Runtime API.

A V0 continua sendo a versão aprovada em `main` até a promoção da V1.

## JrBot V1 — funcionalmente concluída

Objetivo: permitir que uma pessoa controle o JrBot pelo navegador na rede local, usando o painel servido pelo próprio ESP32-S3.

Versão final da linha: **`JrBot_V1.6.2`**.

PR #20 foi concluído e integrado em `develop` em 2026-09-14.

### Revisão V1.6.2 — instalador e fluxo de branches

A revisão `JrBot_V1.6.2` consolida uma melhoria de manutenção sobre a `JrBot_V1.6.1`:

- descoberta dinâmica das branches remotas ativas pelo `INSTALAR.bat`;
- instalação e troca direta para branches de desenvolvimento e correção;
- exclusão de `archive/*` do menu de versões;
- identificação da branch atual no seletor;
- remoção da lista fixa de apenas `main`, `develop`, `v1` e `v2`.

Não houve alteração funcional do PTT ou do caminho de áudio nesta revisão.

Entregas consolidadas:

- painel interno servido pelo próprio JrBot;
- interface responsiva em PC e celular;
- estado de firmware, Wi-Fi, IP, face, áudio e microfone;
- controle de expressões e volume;
- gravação do microfone do JrBot;
- reprodução e download das gravações;
- envio de mensagens de áudio do celular/computador para o JrBot;
- PTT half-duplex entre celular e alto-falante;
- câmera OV5640 com foto e modo ao vivo;
- resoluções e qualidade JPEG selecionáveis;
- orientação padrão da câmera em 90 graus;
- configuração e diagnóstico de Wi-Fi;
- DHCP como comportamento padrão de rede;
- HTTPS local para APIs seguras do navegador, incluindo microfone;
- arbitragem do barramento I2S compartilhado entre microfone e reprodução.

### Fase atual da V1

A implementação funcional está encerrada, mas a V1 ainda não foi promovida para `main`.

```text
v1 fechada
   ↓
develop com JrBot_V1.6.2
   ↓
uso real / observação
   ↓
correções de campo necessárias
   ↓
validação final
   ↓
main
```

Durante essa fase, o objetivo é evitar novo crescimento descontrolado de escopo. Melhorias devem ser classificadas entre:

1. **fix obrigatório para promoção** — defeito que prejudica a experiência normal ou estabilidade;
2. **melhoria V1.x** — refinamento importante que pode entrar antes ou depois da promoção, conforme risco;
3. **V2 ou posterior** — funcionalidade que muda o objetivo da versão e não deve atrasar a promoção da V1.

A investigação do chiado do alto-falante está registrada na Issue #19 e deve ser tratada como correção isolada enquanto a V1 é observada em uso real.

A frente de impressão 3D/corpo está registrada na Issue #22 e evolui em paralelo, bloqueando uma versão apenas se houver impacto direto na validação física.

## JrBot V2 — linha preservada

Objetivo: acionar as mesmas capabilities do robô por voz local.

O trabalho de Playground, threshold, calibração, estabilidade e latência está preservado em `v2`, PR #21 para `develop`.

A V2 permanece pausada enquanto a V1 passa por observação, correções necessárias e promoção para `main`.

Quando retomada, a V2 deve reutilizar as capabilities já consolidadas na V1, sem criar caminhos paralelos para hardware.

## JrBot V3

Objetivo: permitir que software externo controle as mesmas capabilities sem depender do painel ou da voz.

A V3 é definida pelo tipo de integração, não por um transporte específico. A Runtime API pode usar HTTP/IP, Serial ou outro transporte compatível.

A existência da Runtime API atual não significa que a JrBot V3 esteja concluída como produto.

## JrBrain

Depois da fundação V1-V3, o projeto avança para memória, identidade, personalidade, contexto, LLM, relacionamento e Skills. Esse sistema deve continuar usando apenas as capabilities seguras disponibilizadas pelo firmware.

## Promoção

Fluxo atual:

```text
V0 aprovada em main

v1 -- concluída --> develop
                      ↓
              observação / fixes
                      ↓
               validação final
                      ↓
                    main

Depois:
v2 -> develop -> main
v3 -> develop -> main
```

`main` representa a última versão de produto aprovada. `develop` integra a versão candidata antes da promoção. Implementação, build, teste de bancada e validação física continuam documentados separadamente.
