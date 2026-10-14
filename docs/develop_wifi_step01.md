# develop_wifi: etapa 01 — busca de redes pelo Serial

Base aprovada: `JrBot_V1S_APP_03` da main. O teste em DHCP foi confirmado pelo
usuario e pelo log `JrBot-painel-log (16).txt`: IP 192.168.0.123,
`WEBRTC_CONNECTED`, `DATA_CHANNEL_OPEN`, 50 quadros JPEG enviados e teste de
audio `ESP_OK`. Esse IP pode mudar na proxima concessao DHCP.

Versao desta etapa: `JrBot_V1S_WIFI_STEP01`.

## Alteracao

- Botao **Buscar redes Wi-Fi** em **Configurar Wi-Fi pelo Serial**.
- Lista sob demanda de ate oito pontos de acesso, com nome, sinal e protecao.
- Selecionar preenche o nome da rede; informar senha e salvar continuam manuais.
- Busca permitida somente com a estacao ja conectada, sem reinicio pendente,
  em modo idle. Pare a Live e desligue o autonomo antes da busca.
- Se a placa estiver desconectada, o campo manual de SSID continua disponivel.
- Comando `wifi_scan`; RAM interna livre e maior bloco antes/depois em
  `JR_WIFI_SCAN_HEAP`. A lista temporaria do driver e liberada ao terminar,
  inclusive se a leitura falhar. Nao cria tarefas nem modifica a conexao/NVS.

Esta etapa usa apenas o driver nativo existente. Firmware da camera, audio,
WebRTC, SDKCONFIG e inicializacao do Wi-Fi permanecem os da base aprovada.
Ainda nao inclui multiplas redes, troca automatica ou mDNS.

## Teste na placa

1. Instalar a branch `develop_wifi` e confirmar `JrBot_V1S_WIFI_STEP01` no painel.
2. Conectar a Serial, manter a Live parada e clicar **Buscar redes Wi-Fi**.
3. Repetir a busca tres vezes e verificar os registros de RAM antes/depois.
4. Selecionar a rede atual, informar novamente a senha, manter DHCP e salvar.
5. Reiniciar, consultar o IP recebido e acessar o painel HTTPS nesse IP.
6. Testar foto, Live com video e audio nos dois sentidos, parar e abrir novamente.
7. Com a Live ativa, solicitar uma busca: deve recusar sem interromper a Live.
8. Baixar o log. So avancar para multiplas redes depois da aprovacao desta etapa.

## Verificacao local

`python tests/wifi_scan/test_native_scan.py`: compila o codigo C real com driver
simulado; verifica limites, nomes de 32 bytes, respostas pequenas, bloqueio em
Live/autonomo, rede desconectada, reinicio pendente e limpeza em sucesso/erro.
Usa AddressSanitizer/UndefinedBehaviorSanitizer; o driver simulado verifica a
liberacao da lista, sem depender do LeakSanitizer e seu acesso a /proc.

`node --test tools/jrbot_frontend/tests/wifi-scan.test.mjs`: verifica selecao,
nomes especiais, duplicatas, senha obrigatoria em rede protegida, rede aberta
e erros de busca. A compilacao ESP-IDF e os testes fisicos ainda dependem da
instalacao na placa.

API usada: [documentacao oficial Espressif](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32s3/api-reference/network/esp_wifi.html).
