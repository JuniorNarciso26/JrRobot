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
5. Digite **ouvir** e ENTER. Aguarde **FALE AGORA** e fale uma palavra ou frase
   curta em inglês, inteira na janela de três segundos.
6. Espere o processamento. Procure **JR_STT_TEXT:** seguido do texto reconhecido.
7. Repita `ouvir`. Para encerrar o IDF Monitor, use **Ctrl+]**. Para recuperar
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
maior bloco interno livre e PSRAM livre. O tempo de captura (três segundos) é
separado do processamento. Não existe pontuação de confiança de transcrição no
resultado exibido; o campo `score` da API original é para wake phrases, não para
precisão geral, por isso não o usamos como confiança.

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
  Conversão com deslocamento 14 e saturação, como o microfone validado do JrBot.
- A entrada é capturada e o canal I2S liberado antes da inferência. Buffer PCM
  limitado a 96000 bytes em PSRAM. Não há detecção automática de início/fim de fala.
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
48000 amostras, saturação e liberação dos recursos em captura normal e falha.
Outro teste confirma preparação repetida e recusa de fontes com alterações.
Isso não substitui compilação Xtensa/ESP-IDF e teste no microfone real, ainda não
executados neste ambiente. Não foi medida precisão ou latência no JrBot.

Se a transcrição funcionar no hardware, a próxima etapa será encapsular o motor
em um módulo de escuta ativável. A coexistência com WakeNet, vTTS, Wi-Fi, câmera
ou OLED ainda não foi validada; a `main` permanece preservada.
