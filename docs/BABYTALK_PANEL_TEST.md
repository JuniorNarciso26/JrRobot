# V2.2 — Olá ESP, captura de 5 segundos e texto no painel

Branch de teste: `test/babytalk-panel`. Firmware `JrBot_V2.2_STT_PANEL`.
A aplicação mantém a interface e o controle autônomo do JrBot, com WakeNet
português e BabyTalk inglês. O texto reconhecido só é registrado no log:
não há interpretação de comandos, mudanças de rosto nem resposta falada.

## Instalação e teste

1. Feche o painel/monitor que esteja usando a porta COM.
2. Execute `INSTALAR.bat` (PowerShell: `.\INSTALAR.bat`), selecione
   **test/babytalk-panel** e a porta. O instalador prepara, compila e grava
   aplicação, WakeNet e BabyTalk; abre o painel ao finalizar.
3. No painel, conecte **Serial USB** à mesma porta e ative o **modo autônomo**.
   Mantenha a conexão Serial aberta para receber os eventos no log.
4. Fale **“Olá ESP”**, espere o **bip** e a linha **JR_STT_LISTENING: FALE AGORA**.
5. Fale uma frase curta em **inglês**, por exemplo “my name is Junior” ou
   “change face to happy”. A janela é fixa de **5 segundos**, incluindo pausas.
6. Aguarde **JR_STT_TEXT:** no log. O robô retorna a aguardar “Olá ESP”.
   Aguarde o fim do processamento antes da próxima tentativa.

O instalador usa ESP-IDF 6.1, build e sdkconfig próprios. Não é necessário
editar arquivos nem gravar o modelo separadamente. `build` só compila,
`flash` grava sem abrir painel; `panel` abre somente o painel já instalado.
Para restaurar a versão aprovada completa, reinstale `main` pelo instalador.

## Escopo e memória

O layout aprovado reserva 11 MiB para a aplicação com vTTS e 4 MiB para
WakeNet. Acrescentar a partição de 6 MiB do BabyTalk excederia os 16 MB. **A síntese de voz está desativada
nesta branch experimental**; a saudação automática também não é reproduzida.
A `main` e o teste Serial `test/babytalk-hw04` permanecem separados e preservados.
Esta branch não representa validação de todas as funcionalidades do JrBot.

Layout dedicado, sem OTA: app de 5 MiB em 0x10000, WakeNet `model` de 4 MiB
em 0x510000, BabyTalk `stt_model` de 6 MiB em 0x910000; termina em 0xF10000,
dentro de 16 MiB. O build oficial verifica o tamanho da aplicação e dos modelos.
A troca de layout não altera a posição de NVS e não acrescenta `erase_flash`.

BabyTalk é o mesmo upstream fixado do [teste isolado](BABYTALK_HW04_TEST.md),
com o mesmo SHA-256 do modelo. O adaptador gerado modifica somente o nome da
partição lida pelo motor: `model` → `stt_model`, para não conflitar com ESP-SR.
O clone oficial é validado e não é alterado. Arquivos gerados ficam ignorados.

O pool SIMD de 84,5 KiB de RAM interna é reservado antes de Wi-Fi/OLED; o modelo
é aberto ao ativar o autônomo. PCM de 160000 bytes em PSRAM, 16 kHz mono16,
ganho 1x (deslocamento 16), validado no teste isolado sem clipping digital.
O motor e o buffer PCM são fechados ao desligar o autônomo; o pool reservado
permanece disponível até reiniciar a placa. A tarefa de voz tem 16 KiB de pilha.
O watchdog de tarefas é desativado somente no sdkconfig experimental, seguindo
o teste BabyTalk; Wi-Fi, câmera e inferência simultâneos ainda exigem validação.

Microfone e alto-falante compartilham pinos I2S: fecha o microfone, dá o bip,
reabre e descarta 128 ms de transiente, anuncia captura, grava 5 segundos,
fecha o microfone, transcreve e reabre para WakeNet. Não escuta durante inferência.
Desligar o autônomo cancela a captura; inferência já iniciada termina, mas não
publica texto após o cancelamento. Captura incompleta não é transcrita.

## Logs e critérios do teste

- `JR_STT_READY`: modelo e buffer disponíveis ao ativar o autônomo.
- `JR_STT_LISTENING`: início da janela fixa de 5 s.
- `JR_STT_CAPTURED`: 80000 amostras e pico do PCM.
- `JR_STT_PROCESSING`: inferência local.
- `JR_STT_TEXT`: texto reconhecido ou `(vazio)`; textos maiores são continuados
  em `JR_STT_TEXT_CONT` para caber no transporte Serial.
- `JR_STT_TIME`: tempo de inferência e quantidade de frames.
- `JR_STT_ERROR` / `JR_STT_CANCELLED`: falha ou cancelamento, sem ação verbal.

Não há VAD ou rejeição de silêncio nesta janela fixa; ruído/silêncio podem gerar
texto incorreto. Não há pontuação de confiança nem execução de comandos.
Envie o que falou e o bloco de log correspondente. Repita algumas vezes,
incluindo silêncio, e teste desligar o modo durante a captura.

Validação local: preparação com fonte/modelo oficiais verificados, partições sem
sobreposição dentro dos 16 MB, configuração repetível e captura C real compilada
em host com doubles de I2S e inferência. Confere 80000 amostras, ganho, publicação
do texto, falha de leitura e cancelamento. Os testes não substituem compilação
Xtensa/ESP-IDF nem o teste físico de coexistência WakeNet/BabyTalk/painel.
