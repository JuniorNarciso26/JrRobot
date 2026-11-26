# JrBot_V2.1 — current offline voice integration

**November 26, 2026: owner-approved develop promoted to main.** Current behavior
is native PCM-block streaming for manual text in Voz (<=1200 UTF-8 bytes,
<=60 seconds of audio subject to PSRAM), a fixed ~8-second audio reserve plus
2-second hold (short completed utterances skip the hold), and the cached wake
greeting. The JrBot name pronunciation is approved as “Jay Ar Bot”. Install
main using INSTALAR.bat; main and develop share the ESP-IDF 6.1.1/vTTS profile.
See [benchmark and app test details](TTS_LONG_BENCHMARK.md) and
[Project Status](../01-history-roadmap/PROJECT_STATUS.md) for current evidence,
limits and remaining work. The dated notes below preserve development history;
their old 180-byte/fully buffered limits do not describe the current release.

## Historical integration notes (superseded by the release above)

# JrBot V2.1.3S - vTTS pt-BR integrado na main e develop

**Branch:** main (release aceita) e develop (proximos experimentos).
**Status 24/11/2026:** o usuario aprovou no hardware HW04 o candidato
V2.1.3D_TTS_FAST, commit 7854de30, com beep, OLED Love, voz vTTS
(espaco PCM em PSRAM 74.042 bytes), cache hit e retomada do microfone.
A versao principal passa a chamar-se V2.1.3S_TTS_FAST; main e develop
usam ESP-IDF 6.1.1. O log confirma HTTPS escutando na porta 443,
mas tambem erros TLS -0x7780. Live/WebRTC nao teve nova regressao completa
registrada; leitura longa com streaming nao faz parte desta release.

## Correcao develop V2.1.5D TTS BUFFERED

**Atual:** JrBot_V2.1.5D_TTS_BUFFERED. A main continua V2.1.3S em bab09039.
O teste fisico V2.1.4D falhou em continuidade: log (53) S RTF 3.559, M RTF
7.700, ambos com 14 episodios de FIFO vazia e avisos task_wdt IDLE0. A analise
de silencio do video (limiar -35 dB, minimo 0.4 s) mostra pausas repetidas de
aproximadamente 2.5 s; nao permite atribuir quais palavras foram perdidas.

A V2.1.5D remove o player concorrente e os cortes de texto 64/128 bytes.
Passa o texto completo para vtts_text_to_ids_ex, preservando normalizacao e
segmentacao nativas. Gera todo o PCM antes de abrir o canal fisico de reproducao;
reserva logica I2S continua bloqueando outras fontes durante o trabalho.
Ajustes do decoder voltam a 12 frames iniciais / 128 seguintes, como na release.

Aceita ate 1.200 bytes de texto; PCM limitado a 60 s ou maior bloco PSRAM livre
menos 256 KiB, o menor dos dois. Alocacao pode falhar por memoria/concorrencia;
nesse caso nenhum prefixo e tocado. Exceder capacidade gera ESP_ERR_INVALID_SIZE
antes de abrir o canal fisico: o painel pede para reduzir o texto. Modelo/arena
sao liberados ao terminar. O cache de despertar mantem seu limite de 12 s e o
caminho separado aceito. Beep/Love e devolucao I2S ao microfone permanecem.

Sintese manual inicia em tskIDLE_PRIORITY no CPU0, para que IDLE0 tenha chance
de executar mesmo durante frontend/inferencia sem callbacks PCM; antes da
reproducao sobe a prioridade para 5. Watchdog nao e desativado nem tem timeout
ampliado. Isso precisa de teste fisico. Referencias:
[FreeRTOS IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos_idf.html),
[Watchdogs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/wdts.html).

GET /voice mantem last_first_audio_ms (pedido ate primeira escrita I2S),
last_synth_ms, last_rtf e last_total_ms. last_underruns fica zero no caminho
bufferizado; zero nao e evidencia de ausencia de falha fisica de DMA.
O player drena 5 blocos de 256 frames silenciosos num DMA de 4x256 antes de fechar.

**Teste:** INSTALAR.bat → develop, confirmar versao e SHA, desligar Live e
Autonomo, ativar/salvar Voz com Lite S e repetir a frase do video. Verificar fala
completa/sem interrupcoes, inicio depois da preparacao e ausencia de task_wdt.
Depois testar texto maior e overflow, falha de memoria, WakeNet/cache/beep/Love,
retorno do microfone, camera e Live. Espera inicial maior e prevista: esta
correcao prioriza continuidade e nao promete reduzir o tempo de geracao.

**Verificado aqui:** host C com codigo real de synth_and_play/on_pcm/player e
doubles de frontend/inferencia/FreeRTOS/I2S: texto inteiro entregue uma unica
vez, nenhuma escrita antes de gerar tudo, amostras completas em ordem, final
drenado, overflow sem tocar prefixo, falhas de alocacao/I2S e limites de cache.
Sintaxe C com headers upstream pinados e sem vTTS, JS e guardas de wake passaram.
**Pendente:** build completo ESP-IDF 6.1.1 (ausente neste ambiente) e HW04.

## Historico rejeitado: develop V2.1.4D TTS STREAM

A partir de 24/11/2026, a develop identifica-se como **JrBot_V2.1.4D_TTS_STREAM**.
A main permanece na versao estavel acima. Os limites de 180 bytes e PCM completo
nas secoes historicas abaixo descrevem a release V2.1.3S.

- Fala manual aceita ate **1.200 bytes UTF-8**. Divide o texto em palavras,
  mirando 64 bytes na primeira parte e 128 nas seguintes; palavras maiores ficam
  inteiras. A frontend nativa mantem sua divisao em fonemas/sentencas.
- O callback PCM envia amostras para FIFO de **32 KiB em PSRAM**; tarefa I2S
  separada consome durante a inferencia. O primeiro bloco tem 12 frames e os
  seguintes crescem ate 32. Modelo, lexico e arena sao reutilizados entre partes;
  nao se guarda a fala inteira. Limite de 300 segundos de amostras por pedido.
- O player mantem um canal I2S durante toda a fala, drena o final antes de fechar
  e sinaliza conclusao antes de liberar memoria, busy e dono logico do I2S.
  Falhas podem deixar um prefixo ja reproduzido, com last_error reportado.
- Cache do despertar continua no caminho aprovado, com 12/128 frames e clipe
  de ate 12 s; preparacao nao abre I2S. Beep, Love e retorno do WakeNet mantidos.
- JSON HTTPS agora tem limite de 7.456 bytes para aceitar escapes; buffer no heap
  evita aumentar a pilha HTTP. Texto decodificado continua limitado a 1.200 bytes.
- `JR_TTS_FIRST_AUDIO latency_ms` e `/voice.last_first_audio_ms`: desde a fila do
  pedido ate a primeira escrita I2S bem-sucedida. Nao mede som no alto-falante.
- `last_synth_ms` e `last_rtf` excluem tempo de envio/bloqueio da FIFO;
  `last_total_ms` cobre a execucao ate o player terminar.
- `JR_TTS_STREAM_DONE underruns` / `last_underruns`: episodios de timeout de 40 ms
  aguardando PCM depois que a reproducao comecou; aproximacao de falta de audio,
  nao contagem exata de underruns fisicos do DMA.

**Validacao feita aqui:** `python -m unittest discover -s tests/tts` compila o
codigo real de callbacks/player com doubles pthread/FreeRTOS/I2S e verifica
inicio antes do fim do produtor, ordem sem perdas na volta da FIFO, drenagem,
erros I2S, falhas de alocacao/tarefa, limites do cache e fronteiras UTF-8/palavras.
Checks de sintaxe C com headers vTTS pinados e com TTS desabilitado, sintaxe JS,
6 testes de wake e 9 de instalacao tambem passaram. Nao e build ESP-IDF.
**Pendente:** compilacao completa com ESP-IDF 6.1.1 (ausente neste ambiente),
teste fisico HW04, medidas comparativas e regressao Live/TLS.

**Teste no robo:** INSTALAR.bat → develop. Confirmar a versao e o SHA no boot.
Desligar Autonomo e Live, abrir HTTPS → Voz, ativar e salvar TTS com Lite S,
speed 1.0 e mesmos ajustes anteriores. Enviar primeiro frase curta, depois texto
com varios periodos, acentos e mais de 180 bytes. Guardar log de primeiro audio,
RTF, pausas, resultado e memoria; repetir em M apenas depois do teste S.
Testar Olá ESP separadamente com Autonomo ligado e confirmar cache hit, beep,
Love e retorno da escuta. Comparar com main usando o mesmo texto curto e ajustes.
A geracao medida anteriormente tinha RTF > 1: o streaming antecipa o inicio,
mas pausas ainda sao possiveis e blocos menores podem reduzir a vazao de sintese.

## Objetivo e controles

Nova aba Voz em https://jrbot.local, ao lado de Controle, Memoria e Wi-Fi.
Permite: ligar/desligar a sintese, escolher modelo Lite S / Quality M,
volume (0-100), gain digital (0-24 dB), velocidade (0.7-1.6, maior e mais
lento), expressividade (0.5-2.5, depende de prosodia), debuzz (0-12 dB)
e digitar frase de teste de ate 180 bytes UTF-8.

O sintetizador offline e baseado no runtime C do projeto tuanpmt/vtts,
Apache-2.0, fixado no commit 010cd22fd51f58dfa2414824cb8d6345e70cafc6;
modelos nu-pt S 1.222.368 bytes e M 2.607.840 bytes da release v0.11.0.
Funciona com as frases digitadas, sem Internet. Ainda nao e STT nem assistente
conversacional. Na release, detectar Ola ESP aciona o beep de 880 Hz / 180 ms e o
rosto Love por 3 s, depois reproduz "Ola! Eu sou o JrBot." e retoma
WakeNet. No log do HW04 a resposta funcionou, mas sintetizar 1,678 ms
de audio levou 5,808 ms (RTF 3,46) e disparou um aviso de watchdog
IDLE0. V2.1.3S_TTS_FAST inclui a geracao antecipada do PCM uma vez
por boot, logo ao ativar Autonomo, usando Lite S (22050 Hz, 1.0x).
Aproximadamente 74 KB ficam na PSRAM, sem manter modelo/arena na RAM.
Nas proximas ativacoes de Ola ESP, usa diretamente o PCM em cache,
aplica volume/ganho/filtro atuais e libera o I2S apos tocar. Mesmo na
falha da sintese, beep, rosto e retorno da escuta sao preservados.
A preparacao ocorre numa tarefa separada que cede CPU periodicamente,
para evitar starvation do IDLE0; eventos USB nao esperam flush de 300 ms.
O dono confirmou que as respostas seguintes ao cache ficaram rapidas no HW04.

## Seguranca da integracao

- Fala sempre OFF apos boot. Ajustes (exceto ativacao) persistem em NVS.
- Runtime e modelos carregados sob demanda na PSRAM, liberados apos falar.
- Tarefa de audio separada para nao bloquear requisicoes HTTP.
- Bloqueio JR_I2S_OWNER_TTS compartilha o mesmo gerente de recursos da Live
  e do microfone. Para testar o botao manual, desligar Autonomo/WakeNet e Live.
  Na resposta ao despertar, o microfone e fechado antes do beep e TTS e
  reaberto apenas depois da reproducao; nunca tocar com I2S do microfone ativo.
- GET /voice fornece parametros, memoria e RTF.
- POST /voice requer HTTPS, JSON, cabecalho X-JrBot-Voice: 1.
- Componentes desenvolvidos em firmware/module_tts/ e firmware/main/voice_panel.js.
- Codigo upstream e modelos locais ficam em firmware/vendor/, ignorado pelo Git.
  Sua licenca LICENSE e o arquivo NOTICE acompanham a preparacao local.

## Instalacao automatica no Windows, ESP-IDF 6.1.1

Depois de instalar ESP-IDF 6.1.1 pelo EIM uma unica vez, abra o arquivo
**INSTALAR.bat com duplo clique** na pasta local do JrBot. O instalador:

1. Consulta branches, atualiza a branch selecionada (main ou develop) sem sobrescrever alteracoes locais.
2. Ativa e valida o ESP-IDF 6.1.1.
3. Reutiliza automaticamente o certificado HTTPS local de um clone vizinho,
   ou gera certificado autoassinado para jrbot.local, sem publicar chave privada.
4. Prepara os fontes e modelos vTTS S/M (reutiliza o teste isolado quando disponivel).
5. Grava o SHA Git atual no firmware (cabecalho local ignorado pelo Git),
   aplica as configuracoes mDNS/PSRAM e compila.
6. Solicita a selecao da porta COM; apos o build bem-sucedido, grava
   automaticamente nessa porta, sem solicitar uma confirmacao adicional.
7. Abre PAINEL.bat apos a gravacao: navegador em http://127.0.0.1:8765.

O certificado e autoassinado e pode provocar aviso no navegador. Isso NAO
instala automaticamente uma autoridade de certificacao confiavel no Windows.
No dispositivo, o painel web HTTPS usa https://jrbot.local quando mDNS resolve;
o IP via HTTP e uma opcao de diagnostico, mas nao autoriza comandos HTTPS-only.
O servidor desktop 127.0.0.1:8765 e distinto do portal embutido no ESP32.
Nunca exponha a API HTTPS do robo diretamente a internet publica.

**Modos avancados (opcionais):** INSTALAR.bat build apenas compila;
INSTALAR.bat flash compila e grava, sem abrir painel;
INSTALAR.bat panel apenas abre o painel de desenvolvimento.

O preparador reutiliza a instalacao local em
Desktop/JrBot_VTTS_Test/experiments/vtts-hw04/.work/vtts, se encontrada;
senao obtem o mesmo commit upstream, baixa modelos v0.11.0 e gera
lex_en.bin. Requer Git, Python e conexao na primeira preparacao.

A compilacao usa ESP-IDF 6.1 e o parametro
-D JRBOT_ENABLE_VTTS=ON, com build isolado b-main-vtts (main) ou
b-develop-vtts (develop). Ambas usam ESP-IDF 6.1.1.

O build completo da develop passou no HW04 em 23/11/2026 e o teste de
fala/cache foi aceito pelo usuario em 24/11/2026. O instalador agora
seleciona a COM e grava automaticamente apos compilar; portanto, executar
INSTALAR.bat com a placa conectada substitui o firmware no ESP32. Se o
build falhar, a gravacao NAO sera executada. A branch main recebe a versao aceita; futuras alteracoes experimentais
ficam somente em develop.

**HTTPS privado:** firmware/main/jr_https_material_local.h NAO entra no Git.
Na primeira execucao, tools/prepare_https_develop.py procura uma copia
local valida (por exemplo, outro clone no Desktop); se nao achar, cria
certificado/chave novos apenas neste computador. Nas execucoes seguintes
reutiliza o arquivo e nao troca a chave. O build com vTTS em ambas as branches
EXIGE esse header e falha se ele nao habilitar HTTPS: a compilacao nao deve
mais produzir silenciosamente um binario sem HTTPS. Ao ligar, procure a
mensagem JR_HTTPS_MATERIAL_COMPILED e em seguida JR_HTTPS_READY. Se
aparecer https_start ou https_register com erro, investigue o servidor.
O instalador mostra [GRAVADO] somente apos idf.py flash confirmar sucesso.
A versao foi alterada para JrBot_V2.1.3S_TTS_FAST e o build registra
a revisao exata do GitHub como git=XXXXXXXX em BOOT, no comando
version e em /status (JR_STATUS). O instalador usa o mesmo SHA curto de
8 caracteres. Verifique versao, git e Compile time antes de afirmar que
a atualizacao esta instalada. Nunca publique o header HTTPS nem abra
POST /voice na internet publica.

No painel HTTPS escolha Voz, desligue Autonomo e Live, ative o TTS,
selecionando inicialmente Lite S, volume 75, gain 6 dB, speed 1.0,
expression 1.0, debuzz 0. Envie uma frase. Compare depois com M.

## Particoes e limites fisicos

ESP32-S3 HW04: flash 16 MiB / PSRAM 8 MiB.
O teste standalone vTTS gerou app ~5.83 MiB **sem** camera,
WakeNet nem WebRTC. Nao comprova que tudo cabera junto.

A release reserva:
- app factory 0x10000 + 0xB00000 (11 MiB);
- model ESP-SR 0xB10000 + 0x400000 (4 MiB);
- fim em 0xF10000, dentro dos 16 MiB.

Os modelos vTTS e lexico ficam embutidos no app para o primeiro
teste. A particao model continua para WakeNet. Se o firmware final
nao couber, interrompa e investigue mover modelos para particao de
dados ou manter apenas o S; NAO remover funcoes estaveis da main.

## Validacao de release: confirmacoes e pendencias

- [x] ESP-IDF 6.1.1 detectado.
- [x] Build completo concluido em 23/11/2026; particoes geradas e aceitas pelo build.
- [x] Boot no HW04, PSRAM 8 MB, Wi-Fi conectado, IP e portal HTTP observados.
- [ ] HTTPS com certificado local, OLED, camera e WebRTC sem regressao.
- [x] WakeNet Ola ESP reconhece, emite beep e mostra Love por 3 s no HW04.
- [x] V2.1.3: log JR_TTS_WAKE_CACHE_PREPARE done=ESP_OK e cache pronto antes de nova chamada.
- [x] V2.1.3: JR_TTS_WAKE_CACHE_HIT registrado; usuario confirmou resposta rapida na segunda chamada.
- [ ] V2.1.3: sem task_wdt IDLE0 durante prewarm; Wi-Fi e Live sem regressao.
- [x] vTTS responde "Ola! Eu sou o JrBot." apos beep no HW04 sem reboot nos logs.
- [ ] Na falha de TTS, beep, OLED e escuta continuam funcionando.
- [x] Log confirma microfone/I2S liberado durante fala e retomado apos reproduzir.
- [ ] Voz fala pelo MAX98357A, com volume e gain e modelos S/M.
- [ ] Ajustes persistem, ativacao OFF apos boot.
- [ ] Fala manual bloqueada durante Live/WakeNet; unica excecao e resposta WakeNet apos fechar microfone.
- [ ] Medicoes antes/durante/depois de RAM interna e PSRAM.
- [ ] Sem reboots, WDT, estouro de memoria ou chiados inaceitaveis.
- [ ] Teste de recuperacao usando main aprovado.
- [x] Autorizacao explicita para promocao da versao aprovada para main em 24/11/2026.

Licenca MIT do JrBot e Apache-2.0 do vTTS sao mantidas.
Para distribuicao comercial de firmware/modelos, confirmar licencas
dos artefatos binarios e manter avisos de direitos autorais.
