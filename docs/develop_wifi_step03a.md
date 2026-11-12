# STEP03A — teste do envio de video com mDNS ativo

Base STEP03: `22fa60a5caba98d8bcd54bd97012faabe2d9898c`.
Versao de teste: `JrBot_V1S_WIFI_STEP03A`.

## Evidencia do log 23

A placa continuou em DHCP no IP 192.168.0.123. Capturou foto XGA e completou
ICE/DTLS, audio PCMA e abertura do DataChannel `jrbot-video`. A falha aparece
depois, no envio: 329 registros `esp-aes: Failed to allocate memory`, seguidos
de `PEER_DEF: Write fail -1` e 251 descartes de video por buffer cheio. Nao e
uma evidencia de falha de resolucao de nome ou de captura do sensor.

Comparacao com o log 22, que enviou 50 quadros e nao registrou falha AES:

| Momento | STEP02: RAM interna livre / maior bloco | STEP03: RAM interna livre / maior bloco |
| --- | --- | --- |
| Antes do peer | 74843 / 31744 bytes | 67847 / 31744 bytes |
| Abertura do canal de video | 23471 / 11264 bytes | 16463 / 8192 bytes |

Ha aproximadamente 7 KB a menos de RAM interna no teste STEP03. O log 23
comeca depois da inicializacao do mDNS, portanto nao inclui os registros
`JR_MDNS_MEMORY` e nao permite atribuir byte a byte essa diferenca.

A [FAQ oficial da Espressif](https://github.com/espressif/esp-faq/blob/master/docs/en/software-framework/wifi.rst)
identifica essa mensagem AES como falha de alocacao DMA. No
[driver ESP-IDF 5.5](https://github.com/espressif/esp-idf/blob/v5.5/components/mbedtls/port/aes/dma/esp_aes_dma_core.c),
o caminho de realinhamento/copia pode alocar buffers de entrada e saida DMA.
No ESP32-S3 o limite de cada copia e 1600 bytes; nao e correto assumir que o
driver tenta alocar um unico buffer AES de 10 KB. O tamanho da mensagem enviada
tambem afeta buffers/filas do transporte. Reduzir o tamanho das mensagens e
uma tentativa controlada de reduzir o pico total, nao uma correcao fisica ja
confirmada para a falha DMA.

## Alteracao isolada para teste

- Payload de cada mensagem JPEG DataChannel: 10000 para 2048 bytes.
- Mantem o mesmo JPEG, resolucao, qualidade, FPS alvo, protocolo, audio, cifra,
  aceleracao AES, stacks e configuracao mDNS. Nao troca para AES por software
  nem move stacks para PSRAM.
- Cede 1 ms entre partes da mesma imagem para permitir escoamento do transporte.
- O cabecalho de 5 bytes e os 7 bits de sequencia permanecem: ate 128 partes
  transportam os 262144 bytes do framebuffer configurado. Frames maiores sao
  recusados explicitamente, como ja ocorria no limite do protocolo anterior.
- Acrescenta memoria DMA livre/maior bloco aos registros HEAP, um registro
  depois da criacao da tarefa de video e no primeiro erro/depois a cada 50 erros.

## Verificacao

`python tests/video_packets/test_packets.py` compila a funcao real de envio C e
passa seus pacotes ao remontador JavaScript real do portal. Testa quadros de
1, 2047, 2048, 2049, 10000, 65536 e 262144 bytes, incluindo 128 partes, limite
excedido e propagacao de backpressure. Verifica bytes identicos na imagem final
com AddressSanitizer/UndefinedBehaviorSanitizer no teste host. Esse teste nao
mede o alocador DMA, o transporte real ou desempenho na placa.

## Teste fisico necessario

1. Atualizar `develop_wifi` e confirmar STEP03A; iniciar com reinicio completo.
2. Manter a Serial conectada desde o boot para capturar o custo do mDNS.
3. Abrir https://jrbot.local e testar foto, Live equilibrada por 30 segundos e
   audio nos dois sentidos. Parar e abrir novamente.
4. Testar os demais perfis de video e observar FPS/descarte de quadros.
5. Enviar o log: procurar progresso WEBRTC_JPEG, ausencia de erros AES, DMA
   disponivel e comportamento depois de parar/reabrir.
6. Se ainda falhar, comparar apos novo reboot pelo IP https://192.168.0.123,
   mantendo o mesmo firmware. O IP pode mudar por DHCP; usar o exibido no log.

A compilacao ESP-IDF completa e a aprovacao da coexistencia mDNS/Live continuam
dependentes do instalador e do teste no hardware. Se a falha persistir, usar
os registros DMA e um teste com mDNS desligado antes de alterar outros limites.
