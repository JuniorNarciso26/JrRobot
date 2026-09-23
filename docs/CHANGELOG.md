# Changelog do JrBot

Este arquivo registra as mudanças promovidas para a linha de integração do produto.

## JrBot_V1.6.3 — 2026-09-23

Base: `JrBot_V1.6.2`.

### Live local

- substitui o fluxo principal PTT pela Live WebRTC full-duplex;
- áudio PCMA/G.711A 8 kHz mono em `SEND_RECV`;
- vídeo JPEG da OV5640 via WebRTC DataChannel/SCTP;
- sinalização HTTPS local por SSE/POST;
- perfis de vídeo rápido, equilibrado e qualidade;
- mute independente das duas direções;
- monitor de memória durante a sessão.

### Hardware e memória

- valida I2S RX/TX simultâneo no HW04 com BCLK GPIO21 e WS GPIO47 compartilhados;
- mantém mic SD GPIO41 e MAX98357A DIN GPIO42;
- ajusta a política de PSRAM para preservar RAM interna durante ICE/DTLS/SCTP;
- preserva orientação física da câmera em -90°/270°.

### Validação física

Confirmados em hardware real:
- JrBot → celular;
- celular → JrBot;
- vídeo contínuo;
- mute do microfone do celular;
- mute do áudio do JrBot.

### Licenciamento

- adiciona a MIT License ao repositório;
- documenta que contribuições aceitas usam a mesma licença MIT;
- preserva licenças próprias de dependências e componentes de terceiros;
- esclarece que a licença do código não concede automaticamente direitos sobre a marca JrBot.

### Limitações

- AEC/eco acústico adiado;
- comunicação WebRTC pela Internet ainda não implementada;
- eventos ocasionais de backpressure do DataChannel foram observados sem impacto perceptível no teste A/B.

### Histórico absorvido

O hotfix `JrBot_V1.6.2.1` de rotação não foi integrado separadamente; sua orientação correta está incluída nesta revisão.


## JrBot_V1.6.2 — 2026-09-19

Base: `JrBot_V1.6.1`.

### Melhorias

- `INSTALAR.bat` deixou de depender de uma lista fixa de branches;
- o menu passa a consultar todas as branches remotas ativas diretamente no GitHub;
- branches `feature/*`, `fix/*` e `hotfix/*` podem ser selecionadas para instalação e teste;
- branches `archive/*` permanecem ocultas;
- a branch atualmente usada é indicada com `[ATUAL]`;
- o mesmo fluxo continua responsável por sincronizar, compilar, gravar e abrir o painel.

### Compatibilidade

- nenhuma mudança de hardware;
- nenhuma mudança no PTT half-duplex;
- nenhuma mudança na arquitetura de áudio da V1;
- nenhuma mudança de contrato da Runtime API.

### Processo

A partir desta revisão, toda promoção de versão para `develop` deve atualizar também a documentação e este changelog.

## JrBot_V1.6.1

Revisão anterior da linha V1, com a experiência de navegador consolidada, câmera com orientação padrão de 90 graus, áudio/microfone, HTTPS local e interfone PTT half-duplex.
