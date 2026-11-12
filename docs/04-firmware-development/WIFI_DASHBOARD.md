# Wi-Fi — Aba de gerenciamento apos a configuracao inicial

## Investigação de queda intermitente — V1.8.4S / DIAG_01 (`develop`)

Após testes em HW04 em 12/11/2026, a conexão Wi-Fi foi observada funcionando
após reinício, porém o acesso ao painel HTTPS falhou após algum tempo.
O log anterior apresentou \`httpd_uri: uri handler execution failed\`, falhas
de handshake TLS \`-0x7280\`/\`-0x0050\` e perda do monitor Serial. Isso **não**
determina se a causa foi perda de rádio, TLS, heap ou outro subsistema.

Esta alteração é **somente instrumentação**: não muda políticas de reconexão,
os quatro perfis NVS, o portal cativo ou o fluxo de Live. A implementação atual
limita tentativas de conexão quando todas as redes falham e a recuperação de
boot de ~30 s só atua antes de iniciar o portal principal. Uma queda tardia,
portanto, pode deixar o robô desconectado até intervenção; investigar nos logs
antes de alterar a política.

**Linhas novas do Serial (filtrar pelo prefixo):**

- \`JR_NET_ASSOC\`: associação ao AP (SSID realmente usado, BSSID, canal);
- \`JR_NET_IP\`: IP obtido ou evento de perda de IP;
- \`JR_NET_DIAG disconnect_event=\`: hora de atividade, SSID/BSSID do evento,
  código/motivo de desconexão, memória interna e PSRAM disponíveis;
- \`JR_WIFI_RETRY\` e \`JR_WIFI_FAILOVER\`: tentativa, troca e exaustão dos
  perfis;
- \`JR_NET_HEALTH\`: amostra a cada 30 s (SSID de perfil ativo, SSID do AP
  realmente associado, BSSID, RSSI, canal, IP, contagem de quedas, RAM interna
  livre, maior bloco contínuo e PSRAM livre);
- \`JR_HTTP_FAIL\`: URI sem query string, método HTTP, erro de retorno e heap
  quando um handler retorna erro. Erros durante a **negociação TLS** acontecem
  antes da URI e continuam registrados pelo próprio ESP-IDF; correlacionar
  com \`JR_NET_HEALTH\` para observar a memória próxima ao evento.

Nenhuma senha ou conteúdo do corpo HTTP é gravado nos logs.
O build é identificável por
\`build_sp=2026-11-12_JrBot_V1.8.4S_DIAG_01\`.
A versão exibida segue \`JrBot_V1.8.4S\` (candidata somente em \`develop\`).

**Procedimento:** instalar esse build, abrir Serial COM8 no painel e deixar
o robô funcionando pelo menos até reproduzir a falha (sem reiniciar antes de
salvar o log). Testar primeiro parado e depois com a câmera/Live, guardando
os logs completos incluindo o período anterior e posterior à queda.
Comparar \`JR_NET_HEALTH\` antes da falha com \`JR_NET_DIAG\`, \`JR_HTTP_FAIL\`
e \`esp-tls-mbedtls\`. Verificar se \`actual_ssid\` coincide com o
\`active_profile\`. **Validação pendente:** build ESP-IDF 5.5.5,
memória efetiva em HW04 e reprodução de queda. Não promover à \`main\`
antes da aprovação do responsável.

---



**Status:** incorporado na `main` pela [PR #59](https://github.com/JuniorNarciso26/JrRobot/pull/59) em 07/11/2026.  
**Firmware oficial:** `JrBot_V1.8.3S`  
**Origem:** `feature/wifi-dashboard`, baseada na versão `JrBot_V1.8.2S`.

## Funcionalidade

A aplicacao do JrBot (`https://jrbot.local`) passa a apresentar a aba **Wi-Fi**, ao lado de **Controle** e **Memoria**. A configuracao inicial pelo AP `JrBot-Setup-XXXXXX` e os controles por Serial continuam existindo.

A aba mostra, sob demanda:
- status de conexao e nome SSID atual;
- endereco IP, canal e nivel de sinal em dBm com classificacao qualitativa;
- modo atual do robô, para deixar claro quando a busca ou troca de Wi-Fi esta bloqueada;
- ate quatro perfis salvos e qual foi selecionado;
- lista de ate oito redes encontradas em busca **manual**;
- cadastro de novo perfil com SSID e senha;
- selecao de perfil existente para a **proxima inicializacao**;
- botao **Excluir** ao lado de cada rede salva, com confirmacao antes da exclusao;
- confirmacao explicita para reiniciar e aplicar a rede selecionada.

O monitoramento opcional do RSSI ocorre a cada 10 segundos **somente com a aba aberta e visivel**, sem realizar varredura de redes. A tela nunca exibe ou devolve senhas.

## Fluxo de troca de Wi-Fi

1. Abrir `https://jrbot.local` na rede atual e entrar na aba **Wi-Fi**.
2. Consultar sinal/IP. Se desejar, pressionar **Procurar redes** (somente em modo ocioso).
3. Selecionar um SSID da busca ou digitá-lo. Informar senha e clicar **Salvar rede sem desconectar**.
4. Em **Redes salvas**, clicar **Usar esta rede** para tornar o perfil selecionado no NVS.
5. Verificar o aviso de troca pendente e clicar **Confirmar e reiniciar para trocar**.
6. O navegador pode perder o acesso, pois o robô deixa a rede anterior e o IP pode mudar. Conectar o celular/PC à rede escolhida e abrir novamente `https://jrbot.local`.
7. Se a nova rede nao conectar, a rotina de inicializacao existente do JrBot entra em setup apos o tempo limite. A configuracao inicial permanece disponivel como recuperacao.

## Exclusao de rede salva

- Na aba **Wi-Fi**, em **Redes salvas**, pressionar **Excluir** ao lado do SSID e confirmar a exclusao.
- Para uma rede **nao selecionada**, o perfil e apagado imediatamente da NVS. A conexao atual nao cai nem ha reinicio automatico.
- Ao excluir a **rede selecionada**, a interface mostra aviso reforcado: a conexao atual permanece ativa, mas a configuracao selecionada e desfeita para o proximo boot. O JrBot pode entrar no modo de configuracao inicial ao reiniciar. Recomendado: selecionar outra rede valida antes do reinicio.
- A exclusao exige modo `idle`, como as demais alteracoes. A senha nunca e exibida na interface nem no retorno da API.
- Se a rede ainda estiver conectada fisicamente, o monitoramento de SSID/IP continuara mostrando a conexao ate o reinicio; isso nao significa que o perfil apagado foi restaurado.

**Nao ha troca em quente neste MVP:** a conexao atual permanece ate o reinicio para reduzir o risco de queda durante o cadastro. A senha **nao e validada antes da selecao**; uma senha incorreta pode provocar retorno ao setup no reinicio.

## Interface/seguranca

- O JavaScript fica no arquivo `firmware/main/wifi_panel.js`, embutido como texto de programa. Ele cria uma nova aba na navegacao existente do `memory_panel.js`.
- A API fica em `firmware/module_portal/jr_wifi_web.c` e e registrada **apenas no servidor HTTPS** existente:
  - `GET /wifi-manager?view=status` — RSSI, IP, SSID codificado e estado, sem credenciais.
  - `GET /wifi-manager?view=profiles` — ate quatro perfis sem senhas.
  - `GET /wifi-manager?view=scan` — varredura manual, apenas em modo ocioso.
  - `POST /wifi-manager` — acoes `save`, `use`, `delete` ou `restart`, com JSON e cabecalho `X-JrBot-WiFi: 1`. A acao `delete` recebe `id` de 0 a 3 e retorna `deleted: true` em caso de sucesso.
- Operacoes de escrita exigem o modo `idle` e, quando presente, a origem precisa coincidir com o host HTTPS da requisicao.
- As rotas HTTP existentes (`/cmd`) **continuam bloqueando comandos `wifi_*`**; os comandos Serial e o provisionamento por AP permanecem independentes.
- A UI nao implementa autenticacao de usuario nesta etapa: **nao publicar o painel diretamente na Internet** e usar somente rede local confiavel. A protecao de navegador por origem e cabecalho nao substitui autenticacao.
- Senhas de requisicao sao mantidas apenas durante a gravacao e limpas das estruturas temporarias antes da resposta. O NVS armazena as credenciais conforme a implementacao atual do JrBot.

## Memoria e compatibilidade

- Nenhuma tarefa de monitoramento Wi-Fi nova roda na ESP32-S3.
- O RSSI vem de `esp_wifi_sta_get_ap_info`; a busca usa o scanner existente e nao ocorre automaticamente.
- Os perfis reutilizam `jr_wifi_profile_save`, `jr_wifi_profile_use`, `jr_wifi_profile_delete` e o armazenamento NVS existente.
- Os buffers de consulta sao alocados temporariamente na PSRAM e liberados apos cada resposta.
- O codigo do painel esta na flash, carregado e executado pelo navegador. **Verificar o efeito real da API e da pagina sobre a RAM interna e o tamanho final da flash no hardware.**
- A busca de redes exige modo `idle`, pois pode prejudicar audio/video durante a Live.

## Plano de verificacao apos a integracao

1. Compilar no ESP-IDF 5.5 e gravar `JrBot_V1.8.3S` na placa HW04.
2. Confirmar as tres abas **Controle, Memoria, Wi-Fi** no celular e computador.
3. Consultar sinal/IP/SSID conectado e conferir os valores no roteador.
4. Verificar que a busca de redes so executa em `idle`; ela deve ser recusada durante Live/autonomo.
5. Cadastrar um segundo SSID, confirmar que a conexao atual **permanece ativa** e que a senha nao aparece na resposta.
6. Selecionar segundo perfil sem reiniciar; confirmar que a rede atual ainda funciona.
7. Excluir uma rede salva nao selecionada: confirmar remocao da lista e manutencao da rede atual sem reinicio.
8. Excluir a rede selecionada: conferir confirmacao reforcada, permanencia da conexao atual e aviso de configuracao inicial no proximo reinicio. Selecionar outra rede valida antes do reinicio de teste.
9. Confirmar reinicio pelo botao e validar retorno ao `jrbot.local` na segunda rede.
10. Validar recuperacao pela tela inicial quando a rede destino nao esta disponivel.
11. Rodar Live de audio/video, OLED e Mesa Mix para confirmar que nao houve regressao.
12. Comparar RAM interna livre, maior bloco, PSRAM, consumo durante consultas e varredura; verificar tambem tamanho do binario/flash.

**Status:** PR #59 mesclada na `main` e versão `JrBot_V1.8.3S` confirmada no repositório. A compilação ESP-IDF e a validação completa em hardware não foram verificadas nesta documentação; os itens acima permanecem como checklist de regressão. A branch original foi preservada.


## Correção adicional V1.8.4S — autoreinício controlado (develop)

O log físico de 12/11/2026 mostrou que o portal cativo podia iniciar
após ~30 s sem IP antes de a rede alternativa terminar de conectar.
Como o próprio teste confirmou sucesso após reinício manual, o boot
passa a executar **um único reinício automático** na primeira expiração
desse tempo. Se a segunda inicialização ainda não obtiver IP,
oferece a tela de configuração, sem reiniciar em ciclo.
Com IP válido, o marcador de recuperação é limpo. A seleção
manual, o limite de quatro redes e o armazenamento NVS permanecem.
O reinício automático só acontece antes de o portal normal iniciar;
não derruba sessões Live já estabelecidas. Testes ESP-IDF/HW04 pendentes.

## V1.8.4S — correção automática (develop)

Com a V1.8.4S candidata, o boot começa pelo perfil selecionado e pode
alternar automaticamente para outros SSIDs salvos após duas falhas de
conexão por perfil. Quando uma rede alternativa conecta e recebe IP, o
JrBot a grava como preferida para o próximo boot. O painel continua
permitindo seleção manual, exclusão de redes e reinício confirmado. Não
há varredura permanente: o failover ocorre após uma desconexão, sem tarefa
extra. A mudança não foi promovida à main nem validada fisicamente.

Teste de aprovação: grave a candidata, deixe a última rede fora de alcance,
confirme que a outra rede salva conecta e atualiza a seleção; depois repita
com senha antiga incorreta, todas redes indisponíveis, configuração IP fixa,
exclusão de perfil, reconexão com WebRTC e memória interna/PSRAM.
