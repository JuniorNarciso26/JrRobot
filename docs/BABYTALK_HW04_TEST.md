# BabyTalk HW04 — escuta e transcrição em inglês, teste isolado

Branch `test/babytalk-hw04`, criada da `main` V2.1 aprovada. Este teste substitui
somente a aplicação instalada na placa; não é integração com o firmware completo.

## Instalar e testar

1. Feche o painel e qualquer outro monitor usando a porta COM.
2. Execute `INSTALAR.bat`, selecione **test/babytalk-hw04** e a porta do JrBot.
3. O instalador clona a fonte fixada do BabyTalk, verifica o modelo pelo SHA-256,
   compila e grava a aplicação **e o modelo**, sem comandos extras. Internet é
   necessária na preparação; a transcrição na placa funciona sem rede.
4. O monitor Serial abre automaticamente. Aguarde **JR_STT_READY**.
5. Pressione **ENTER**. Aguarde **FALE AGORA** e fale uma palavra ou frase em inglês.
   Outro **ENTER** encerra; uma pausa de **800 ms** depois da fala também encerra.
   A captura termina em até **10 segundos** de áudio retido ou após **5 segundos**
   sem detectar fala. `ouvir` + ENTER continua funcionando.
6. Espere o processamento. Procure **JR_STT_TEXT:** seguido do texto reconhecido.
7. Pressione ENTER novamente após **JR_STT_READY**. Para encerrar o IDF Monitor, use **Ctrl+]**. Para recuperar
   a aplicação completa, execute `INSTALAR.bat` novamente e selecione `main`.

No PowerShell, use `.\INSTALAR.bat`. `build` só compila; `flash` grava sem abrir
monitor; `menuconfig` configura este projeto isolado. O painel/app não é usado
para este teste e o robô não inicia Wi-Fi, câmera, OLED, WakeNet nem síntese de voz.
O amplificador recebe zero no GPIO42; não há bip nem resposta falada.

Comece perto do microfone, em ambiente silencioso. Teste, por exemplo:

| Tentativa | Fala de referência |
|---|---|
| 1 | happy |
| 2 | hello robot |
| 3 | turn on the light |
| 4 | turn off the light |
| 5 | silêncio durante toda a captura |

Faça algumas repetições e envie a referência junto das linhas `JR_STT_TEXT`,
`JR_STT_TIME`, `CAPTURADO` e `heap_internal`. Não há lista de comandos cadastrada:
qualquer texto retornado é transcrição do motor, sem acionar funções do robô.
Silêncio/ruído podem gerar texto incorreto e devem ser avaliados também.

`JR_STT_TIME` informa tempo total e etapas de extração de características,
inferência e decodificação. O log também mostra pico, saturação do PCM, RAM interna,
maior bloco interno livre e PSRAM livre. O tempo de captura é variável e separado
do processamento. `motivo=enter/pausa/limite/sem_fala` mostra por que ela terminou. Não existe pontuação de confiança de transcrição no
resultado exibido; o campo `score` da API original é para wake phrases, não para
precisão geral, por isso não o usamos como confiança.

## Ajustes pelo monitor Serial

Digite o comando e ENTER enquanto aparece `JR_STT_READY`:

| Comando | Efeito |
|---|---|
| `ganho 1` | Padrão; menor amplificação, recomendado para começar |
| `ganho 2`, `ganho 4`, `ganho 8` | Amplifica o PCM; pode aumentar saturação e ruído |
| `sensibilidade 150` | Mais sensível que o padrão 300, detecta áudio mais fraco |
| `sensibilidade 600` | Menos sensível, exige áudio mais forte |
| `pausa 1200` | Aguarda 1,2 s de silêncio antes de encerrar; padrão 800 ms |
| `ajuda` | Exibe controles e configuração atual |

Sensibilidade aceita limiar RMS de 50 a 5000; **menor valor = mais sensível**.
Pausa aceita 400 a 3000 ms. Esses ajustes duram até reiniciar a placa.
Sensibilidade controla o detector de início/fim; ganho altera o áudio enviado ao
modelo. Não são ajustes de confiança ou treinamento do reconhecimento.

`FALA_DETECTADA` confirma a detecção por energia. Sem fala suficiente, aparece
`JR_STT_NO_SPEECH` e o motor não é executado. Depois de encerrar a captura, o robô
processa o áudio e só volta a escutar quando aparecer `JR_STT_READY` e você
pressionar ENTER. Ele ainda não transcreve enquanto você fala nem captura áudio
durante a inferência. Evite começar a próxima frase durante o processamento.

## Fonte e configuração

- Fonte: https://github.com/tlack/babytalk
- Commit fixado: `25c9a73b1d54a48b649b781572ac37fdad761d80`.
- Modelo: `citrinet256_int4.mmrt`, original inglês, **5991232 bytes**.
- SHA-256: `bdb6be064984b86af7c7ead182b2852420edaae252733264cd76f81bad0f8d47`.
- Modelo derivado de **stt_en_citrinet_256_ls**, NVIDIA, **CC-BY-4.0**; convertido
  e quantizado pelo BabyTalk. Código BabyTalk sob MIT. Consulte `LICENSE` e
  `models/README.md` no clone baixado para atribuição e condições completas.
- Somente componentes C `stt_engine`, `mmrt` e `sram_pool`; TTS não incluído.
- Fontes do motor são usadas sem modificação, pelo caminho do clone. O wrapper
  CMake declara explicitamente `espressif/esp-dsp` **1.8.2**, versão do lock upstream,
  inclusive como requisito do componente que usa os seus cabeçalhos FFT.
- Modelo registrado no alvo oficial `flash` por `esptool_py_flash_to_partition`.
- Placa: ESP32-S3, **16 MB flash e 8 MB PSRAM octal**. Clocks de 240 MHz, PSRAM
  de 80 MHz, configuração de cache baseada no upstream.
- BCLK=21, WS=47, MIC=41, canal esquerdo, Philips I2S 32 bits, PCM mono16 a 16 kHz.
  Conversão com deslocamento 16, ganho digital configurável e saturação.
  O ganho inicial de 1x é quatro vezes menor que o anterior para reduzir clipping.
- A entrada é capturada e o canal I2S liberado antes da inferência. Buffer PCM
  limitado a 320000 bytes em PSRAM. Detector simples por energia RMS: três blocos
  consecutivos acima do limiar sinalizam fala; silêncio encerra após a pausa
  configurada. Mantém até 200 ms anteriores à detecção para preservar o início.
  Não é VAD neural e pode confundir ruído com fala.
- O pool interno de 84,5 KB é reservado antes de criar a tarefa de aplicação.
  A inferência usa o motor upstream com suas duas CPUs; não há cache extra de pesos.
- O teste usa o ESP-IDF **6.1** já instalado para o JrBot. O upstream documenta
  5.5.1; a compatibilidade com 6.1 precisa ser confirmada no primeiro build.
- Fontes/modelos/build ficam em `.babytalk-hw04/`, ignorado pelo Git. A preparação
  preserva build, sdkconfig e lock gerados e recusa fontes alteradas ou modelo
  com hash diferente. Não é acrescentado `erase_flash` nem reset do repositório.
- `CONFIG_ESP_TASK_WDT_EN` desabilitado apenas neste teste, seguindo o upstream,
  porque a inferência ocupa ambas as CPUs por períodos longos.

## Validação e próxima etapa

Preparação executada com clone oficial fixado e hash real do modelo verificado.
Teste compila o código C real da captura em host com doubles do I2S: confere pinos,
limite de captura, pausa, ENTER, ausência de fala, ganho, saturação e liberação
dos recursos em captura normal e falha.
Outro teste confirma preparação repetida e recusa de fontes com alterações.
A versão anterior foi compilada e testada pelo usuário na placa: captura de 3 s
e inferência em cerca de 660 ms, com clipping no PCM. Os novos controles ainda
precisam ser compilados e testados no hardware; os testes em host não substituem
a validação Xtensa/ESP-IDF, acústica e de memória durante inferência de 10 s.

Se a transcrição funcionar no hardware, a próxima etapa será encapsular o motor
em um módulo de escuta ativável. A coexistência com WakeNet, vTTS, Wi-Fi, câmera
ou OLED ainda não foi validada; a `main` permanece preservada.
