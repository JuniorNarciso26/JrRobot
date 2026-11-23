# JrBot V2.0 - vTTS pt-BR integrado na develop

**Branch:** develop, sincronizado com main; main permanece intacta.
**Status 23/11/2026:** compilacao COMPLETA com ESP-IDF 6.1.1 confirmada
no computador do projeto. O boot no HW04, PSRAM 8 MB, Wi-Fi, IP,
portal HTTP e mDNS foram observados no Serial. O vTTS integrado,
HTTPS, Live, camera, OLED e WakeNet ainda precisam de testes fisicos
especificos. O teste de vTTS isolado havia funcionado no HW04.

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
conversacional; WakeNet nao dispara falas automaticamente nesta etapa.

## Seguranca da integracao

- Fala sempre OFF apos boot. Ajustes (exceto ativacao) persistem em NVS.
- Runtime e modelos carregados sob demanda na PSRAM, liberados apos falar.
- Tarefa de audio separada para nao bloquear requisicoes HTTP.
- Bloqueio JR_I2S_OWNER_TTS compartilha o mesmo gerente de recursos da Live
  e do microfone. Ao testar, desligar Autonomo/WakeNet e encerrar Live.
- GET /voice fornece parametros, memoria e RTF.
- POST /voice requer HTTPS, JSON, cabecalho X-JrBot-Voice: 1.
- Componentes desenvolvidos em firmware/module_tts/ e firmware/main/voice_panel.js.
- Codigo upstream e modelos locais ficam em firmware/vendor/, ignorado pelo Git.
  Sua licenca LICENSE e o arquivo NOTICE acompanham a preparacao local.

## Instalacao automatica no Windows, ESP-IDF 6.1.1

Depois de instalar ESP-IDF 6.1.1 pelo EIM uma unica vez, abra o arquivo
**INSTALAR.bat com duplo clique** na pasta local do JrBot. O instalador:

1. Consulta branches, atualiza a develop a partir do GitHub sem sobrescrever alteracoes locais.
2. Ativa e valida o ESP-IDF 6.1.1.
3. Reutiliza automaticamente o certificado HTTPS local de um clone vizinho,
   ou gera certificado autoassinado para jrbot.local, sem publicar chave privada.
4. Prepara os fontes e modelos vTTS S/M (reutiliza o teste isolado quando disponivel).
5. Aplica as configuracoes mDNS/PSRAM e compila.
6. Solicita a COM e a confirmacao explicita GRAVAR antes de alterar a flash.
7. Grava a placa e abre PAINEL.bat: navegador em http://127.0.0.1:8765.

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
-D JRBOT_ENABLE_VTTS=ON, com build isolado b-develop-vtts.
O branch main permanece em ESP-IDF 5.5 e nao recebe alteracoes.

O build full da develop passou no HW04 em 23/11/2026. A gravacao ainda
pede a confirmacao GRAVAR, porque substitui temporariamente o firmware
V2.0 no ESP32. A branch main do GitHub nao e alterada.

**HTTPS privado:** firmware/main/jr_https_material_local.h NAO entra no Git.
Na primeira execucao, tools/prepare_https_develop.py procura uma copia
local valida (por exemplo, outro clone no Desktop); se nao achar, cria
certificado/chave novos apenas neste computador. Nas execucoes seguintes
reutiliza o arquivo e nao troca a chave. Se o HTTPS nao iniciar, confirme
JR_HTTPS_READY no Serial e verifique se a compilacao/gravacao ocorreu
apos a preparacao HTTPS. Nunca publique esse arquivo ou abra POST /voice
na internet publica.

No painel HTTPS escolha Voz, desligue Autonomo e Live, ative o TTS,
selecionando inicialmente Lite S, volume 75, gain 6 dB, speed 1.0,
expression 1.0, debuzz 0. Envie uma frase. Compare depois com M.

## Particoes e limites fisicos

ESP32-S3 HW04: flash 16 MiB / PSRAM 8 MiB.
O teste standalone vTTS gerou app ~5.83 MiB **sem** camera,
WakeNet nem WebRTC. Nao comprova que tudo cabera junto.

Develop reserva:
- app factory 0x10000 + 0xB00000 (11 MiB);
- model ESP-SR 0xB10000 + 0x400000 (4 MiB);
- fim em 0xF10000, dentro dos 16 MiB.

Os modelos vTTS e lexico ficam embutidos no app para o primeiro
teste. A particao model continua para WakeNet. Se o firmware final
nao couber, interrompa e investigue mover modelos para particao de
dados ou manter apenas o S; NAO remover funcoes estaveis da main.

## Validacao para aprovar merge (pendente)

- [x] ESP-IDF 6.1.1 detectado.
- [x] Build completo concluido em 23/11/2026; particoes geradas e aceitas pelo build.
- [x] Boot no HW04, PSRAM 8 MB, Wi-Fi conectado, IP e portal HTTP observados.
- [ ] HTTPS com certificado local, OLED, camera e WebRTC sem regressao.
- [ ] WakeNet Ola ESP reconhece normalmente quando TTS esta OFF.
- [ ] Voz fala pelo MAX98357A, com volume e gain e modelos S/M.
- [ ] Ajustes persistem, ativacao OFF apos boot.
- [ ] TTS nao executa durante Live, WakeNet ou I2S ocupado.
- [ ] Medicoes antes/durante/depois de RAM interna e PSRAM.
- [ ] Sem reboots, WDT, estouro de memoria ou chiados inaceitaveis.
- [ ] Teste de recuperacao usando main aprovado.
- [ ] Autorizacao explicita para merge na main.

Licenca MIT do JrBot e Apache-2.0 do vTTS sao mantidas.
Para distribuicao comercial de firmware/modelos, confirmar licencas
dos artefatos binarios e manter avisos de direitos autorais.
