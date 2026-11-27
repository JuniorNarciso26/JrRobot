# V2.2 — integração WakeNet, vTTS e BabyTalk

Branch `test/voice-unified`; firmware `JrBot_V2.2_VOICE_UNIFIED`.
Criada a partir de `main` V2.1 (`cb3c004`) e incorporando o teste BabyTalk/painel
aprovado (`4c065844`). As branches aprovadas permanecem preservadas.

## Instalar e validar

1. Feche painel/monitor que use a porta COM. Execute `INSTALAR.bat`
   (PowerShell: `.\INSTALAR.bat`), escolha **test/voice-unified** e a porta.
2. O instalador prepara as mesmas fontes/modelos fixados de vTTS e BabyTalk,
   configura um build próprio, compila, confere os tamanhos reais e grava
   aplicação + WakeNet + BabyTalk. Não há cópia manual de modelos.
3. No painel, conecte Serial USB e confira versão e commit. Com o autônomo
   desligado, teste a fala manual já aprovada, inclusive os modelos Lite e Quality.
4. Ative o modo autônomo. Fale **“Olá ESP”** e aguarde o **bip**.
   A saudação falada automática foi removida; a reação Love por 3 s é preservada.
   A síntese TTS permanece disponível no painel com o autônomo desligado.
5. Depois de **JR_STT_LISTENING: FALE AGORA**, fale em inglês. A janela continua
   fixa de 5 segundos. Procure `JR_STT_TEXT`, `JR_STT_COMMAND` e o retorno ao WakeNet.
6. Repita algumas vezes. Os aliases happy/sad do teste aprovado continuam ativos.
   Teste desligar e religar o autônomo. Com a voz aguardando o despertar ou com
   o autônomo desligado, teste câmera, OLED, Wi-Fi/painel e Live separadamente.
7. Envie o log e, se a compilação terminar, o arquivo
   `firmware/b-voice-unified/voice_unified_budget.json` com os tamanhos reais.

Não há palavras novas nem novas ações nesta etapa. A síntese de textos longos,
configurações de voz S/M, painel, Wi-Fi, câmera e Live continuam no código.
O objetivo deste build é validar sua coexistência: a compilação Xtensa e o
funcionamento físico do conjunto ainda não foram confirmados neste ambiente.
Para voltar à aplicação aprovada, reinstale `main` pelo instalador.

## Flash: aproveitar espaço antes reservado

O layout anterior reservava 11 MiB para a aplicação e 4 MiB para WakeNet,
deixando pouco espaço para acrescentar BabyTalk. Isso não significava que
todas as partições estivessem ocupadas por dados. A inspeção da fonte oficial
ESP-SR encontrou aproximadamente 800 KiB de dados para o modelo Olá ESP;
o binário produzido pela versão 2.6.2 instalada é conferido após cada build.

| Partição | Início | Capacidade |
|---|---|---|
| Aplicação com os dois modelos TTS S/M e léxico | 0x10000 | 8 MiB |
| WakeNet `model` | 0x810000 | 1984 KiB |
| BabyTalk `stt_model` | 0xA00000 | 6 MiB |

O final é 0x1000000, exatamente 16 MiB. Não há OTA. NVS permanece em 0x9000;
não se acrescenta erase_flash. O BabyTalk original tem 5991232 bytes e seu hash
continua validado. Fontes upstream permanecem fixadas nas revisões aprovadas.

A redução da partição da aplicação precisa ser confirmada pelo binário gerado:
`check_voice_unified.py` verifica as três imagens reais e seus offsets em
`flasher_args.json`, capacidade e limites, e recusa imagem ausente/maior antes
de qualquer gravação. O verificador SIMD do vTTS também continua obrigatório.
Se a imagem combinada exceder 8 MiB, o instalador para com o tamanho encontrado;
a estimativa inicial não substitui essa medição. Nenhum modelo TTS foi removido.

## RAM: alternar motores e devolver buffers

- Durante espera: WakeNet ativo; BabyTalk mantém só o PCM de 160000 bytes na
  PSRAM, sem seu modelo aberto nem arena SIMD reservada em RAM interna.
- Após despertar: fecha o microfone, dá o bip, destrói a instância WakeNet e
  suspende a câmera, liberando seus buffers DMA se ela estava inicializada.
- A confirmação do despertar usa apenas bip, sem chamar TTS nem gerar o cache
  da saudação. O TTS manual mantém seus dois modelos e a liberação de buffers
  aprovada; não há prewarm concorrente com a escuta.
- Captura 5 s com ganho 1x. Fecha o microfone antes de abrir/inferir BabyTalk.
- O pool local usa exatamente `MMRT_S3_BUFFER_BYTES` (**73728 bytes**) e libera
  a alocação ao terminar o motor. Substitui a reserva permanente de 86528 bytes
  do pool upstream, que também atendia outro sintetizador não usado pelo JrBot.
  Prefere RAM interna quando houver bloco contínuo e margem de 32 KiB; caso
  contrário usa PSRAM alinhada a 16 bytes, possibilidade também oferecida pelo
  pool upstream. `JR_STT_ARENA memory=internal/psram bytes=73728` informa a escolha.
  O custo de latência da PSRAM precisa ser medido no hardware.
- A instância BabyTalk é fechada após cada inferência, inclusive em falha ou
  cancelamento. O modelo permanece mapeado em flash; o helper de inferência
  do upstream pode manter sua tarefa/semafóros entre interações.
- Recria o WakeNet, reabre o microfone e libera a câmera para novas consultas.
  Capturas da câmera durante a interação de voz retornam indisponível/ocupado;
  não disputam a arena interna do reconhecimento. Uma foto em andamento pode
  adiar a reserva por até 500 ms; se continuar ocupada, essa captura de voz é pulada.

A configuração experimental usa data cache de 32 KiB em vez de 64 KiB para
disponibilizar mais SRAM. Isso pode aumentar o tempo de inferência e precisa
ser medido. O watchdog de tarefas continua desativado neste experimento como
no teste BabyTalk; o build está isolado de sdkconfig/build da `main`.
Antes da inferência, exige ao menos 32 KiB internos livres e um bloco de 8 KiB
para recursos internos. A arena maior pode usar PSRAM. Sem a margem mínima,
registra `JR_STT_ERROR stage=memory` e retoma WakeNet.

`JR_VOICE_PHASE` informa as transições e memória livre antes da captura;
a saudação automática não é mais executada. Falha de captura não é transcrita.
Sem recuperação de WakeNet/microfone,
a tarefa encerra com erro visível em status, em vez de anunciar escuta ativa.

## Validação local e limites

Testes em host compilam o C real dos aliases, captura e inferência controlada,
incluindo as variantes com motor persistente e sob demanda. Testam cancelamento,
falha de leitura e contador dos rostos. O pool C real é testado para alocação
adiada, tamanho exato e devolução/reutilização. O verificador de flash é testado
com o modelo BabyTalk real, imagens grandes demais e arquivos ausentes. Também
há regressão do script oficial ESP-IDF que descobre dependências CMake.
O ciclo real de resposta WakeNet também é compilado em host com doubles dos
drivers/motores: confirma a ordem de liberação e restauração em sucesso e
falha de captura.

Esses testes não executam os kernels Xtensa, os drivers da placa ou os motores
juntos. Ainda é necessário confirmar tamanho final da aplicação, inicialização,
latência, memória após várias interações, recuperação da câmera e funcionamento
Live no hardware. Não é promoção para `main` nem declaração de estabilidade.

### Resultado físico e ajuste após o log 62

O firmware integrado foi instalado e executou WakeNet, bip, saudação TTS e
captura, mas o BabyTalk não chegou à inferência: `internal=96047`,
`largest=31744`, `need=73728`. Isso comprova fragmentação/limitação do bloco
contínuo, sem comprovar interferência acústica da saudação, que terminou antes
da captura. A saudação foi retirada a pedido do usuário. O pool agora pode
usar PSRAM nessa situação, mantendo TTS no firmware. Os testes C reproduzem
os valores desse log, confirmam a escolha da PSRAM e asseguram que o ciclo
não chama a saudação. A correção ainda precisa ser validada na placa.
