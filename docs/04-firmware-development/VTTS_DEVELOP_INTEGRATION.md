# JrBot V2.1.3S - vTTS pt-BR integrado na main e develop

**Branch:** main (release aceita) e develop (proximos experimentos).
**Status 24/11/2026:** o usuario aprovou no hardware HW04 o candidato
V2.1.3D_TTS_FAST, commit 7854de30, com beep, OLED Love, voz vTTS
(espaco PCM em PSRAM 74.042 bytes), cache hit e retomada do microfone.
A versao principal passa a chamar-se V2.1.3S_TTS_FAST; main e develop
usam ESP-IDF 6.1.1. O log confirma HTTPS escutando na porta 443,
mas tambem erros TLS -0x7780. Live/WebRTC nao teve nova regressao completa
registrada; leitura longa com streaming nao faz parte desta release.

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
