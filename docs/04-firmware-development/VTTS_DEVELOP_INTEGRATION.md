# JrBot V2.0 - vTTS pt-BR integrado na develop

**Branch:** develop, sincronizado com main; main permanece intacta.
**Status:** primeira implementacao no GitHub, ainda NAO compilada nem validada
com o firmware completo. O teste de vTTS isolado funcionou no hardware HW04.

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

## Teste no Windows, usando ESP-IDF 6.1.1

Abra um terminal IDF_PowerShell 6.1.1 e use clone separado:

    git clone --branch develop --single-branch https://github.com/JuniorNarciso26/JrRobot.git JrBot_Develop_Voz
    cd .\JrBot_Develop_Voz
    idf.py --version
    python tools\prepare_vtts_develop.py
    .\INSTALAR.bat build

No menu selecione develop. O preparador reutiliza a instalacao local em
Desktop/JrBot_VTTS_Test/experiments/vtts-hw04/.work/vtts, se encontrada;
senao obtem o mesmo commit upstream, baixa modelos v0.11.0 e gera
lex_en.bin. Requer Git, Python e conexao na primeira preparacao.

A compilacao usa ESP-IDF 6.1 e o parametro
-D JRBOT_ENABLE_VTTS=ON, com build isolado b-develop-vtts.
O branch main permanece em ESP-IDF 5.5 e nao recebe alteracoes.

**Nao grave antes de conferir o build.** Se terminar sem erros e ambos os
modelos e particoes couberem, selecione develop e rode:

    .\INSTALAR.bat flash

A gravacao pede confirmacao GRAVAR, porque substitui temporariamente
o firmware V2.0 do ESP32. A branch main do GitHub nao se altera.

**IMPORTANTE - certificados em clones novos:** o arquivo privado
firmware/main/jr_https_material_local.h NAO faz parte do Git. Um clone
limpo pode iniciar somente HTTP, sem HTTPS/Live nem permissao para
salvar voz. Se ja funciona na instalacao estavel, copie manualmente
esse arquivo local para o mesmo caminho no novo clone, SEM publicar
ou enviar o conteudo ao Git. Se nao existir, confira
docs/04-firmware-development/DEVELOPMENT.md (secao HTTPS) e configure
o certificado local antes do teste da aba. Nunca exponha POST /voice
a internet publica; o endpoint e apenas para a rede local.

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

- [ ] ESP-IDF 6.1.x detectado.
- [ ] Build concluido, app menor que 11 MiB, esp-sr model menor que 4 MiB.
- [ ] Inicializacao, OLED, Wi-Fi, camera e WebRTC sem regressao.
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
