## Atualização V1.8.4S — recuperação automática após teste em HW04 (12/11/2026)

**Evidência física apresentada no log `JrBot-painel-log (38).txt`:** a V1.8.4S
iniciou com `FNarciso 2.4G` selecionada e sem IP; após cerca de 30 segundos
abriu `JrBot-Setup-8A7DA9`. Após reinício manual, conectou automaticamente
à `CPGAS_5G`, obteve `192.168.0.123` e a gravou como rede preferida.
Isso mostra que a recuperação abriu o portal antes de uma conexão alternativa
terminar; não comprova que a senha de `FNarciso` estava errada.

**Correção adicional em `develop`:** quando a tentativa inicial não consegue
IP em aproximadamente 30 s, o JrBot executa **um único reinício automático
por software** e tenta as redes salvas novamente. O marcador
`RTC_NOINIT_ATTR` sobrevive ao reinício por software e impede
loop: se continuar sem IP por mais ~30 s na segunda inicialização,
abre o portal de configuração normalmente. Um boot a frio reinicia
essa cota; uma conexão com IP zera o marcador para um futuro ciclo.
Os perfis e senhas em NVS não são apagados; o reinício não é
disparado depois de o JrBot já ter conexão e serviços ativos.

**Testes:** `python -m unittest discover -s tests/wifi_recovery -v`
isola e compila funções reais do guard de recuperação em mock host.
A nova correção ainda exige compilação ESP-IDF e teste HW04 completo
com desconexão da rede selecionada, rede reserva válida, ambas indisponíveis
e Live após a troca. Não promover à `main` sem esse teste.

---

## V1.8.4S — failover entre redes Wi-Fi salvas (candidata em develop)

A partir desta versão candidata, o JrBot não fica limitado à rede selecionada.
No boot, tenta primeiro o perfil selecionado em NVS. Se houver falha de
conexão, faz uma segunda tentativa e, depois, experimenta cada um dos outros
perfis salvos (até quatro), sem pedir a senha de novo. Ao obter um endereço IP
com a rede alternativa, esta se torna a rede preferida no próximo boot.
Falhas não apagam nem substituem as credenciais. Na ausência de redes
funcionais, a recuperação pelo AP de configuração continua disponível
após aproximadamente 30 s. A troca automática de rede pressupõe uma
desconexão; o fluxo normal não faz varreduras periódicas, preservando memória
e a sessão de áudio/vídeo enquanto a conexão se mantém.

A etapa automática usa as credenciais já gravadas na NVS e não garante que
a senha de uma rede antiga permaneça válida. O log Serial identifica o SSID
tentado e o sucesso, sem registrar a senha. Se for necessário alterar a
senha, usar o portal/painel existente.

**Estado de validação:** alteração de código entregue como candidata na
`develop`; teste de conexão real na placa HW04, memória e regressão de Live
ainda necessários antes de aprovar o merge na `main`.

---

# Provisionamento de Wi-Fi do JrBot V1.8.1S–V1.8.2S

Versão atual em `main`: `JrBot_V1.8.2S`; o provisionamento foi introduzido em `JrBot_V1.8.1S`. O desenvolvimento passou pelo branch
`test/develop_wifi_ram_light_provisioning`, baseado em
`test/develop_wifi_ram_light` e no experimento `test/esp-wifi-provisioning`
(commit `2593892`). A tabela de medição abaixo preserva as versões de teste.
Os componentes ADF do exemplo isolado não são ligados ao firmware do robô:
o JrBot mantém seu gerenciador de Wi-Fi, seus perfis NVS e sua configuração
de memória.

## Uso no aparelho

1. Sem rede Wi-Fi salva, ou se a rede selecionada não responder por cerca de
   30 segundos após ligar, o JrBot abre `JrBot-Setup-XXXXXX`, onde os seis
   últimos caracteres vêm do MAC. As redes salvas não são apagadas.
2. Para trocar de rede com uma configuração já salva, segure o botão BOOT
   (GPIO0) por três segundos enquanto o robô está ligado e ocioso; então
   solte o botão. Ele reinicia no modo de configuração sem apagar a rede anterior.
3. Conecte o celular à rede aberta do JrBot, sem senha. O celular deve
   detectar o portal cativo e oferecer a página de configuração. Se não abrir,
   acesse `http://192.168.4.1` no navegador. Escolha uma rede de 2,4 GHz na
   lista ou digite o SSID, informe a senha e toque em **Testar conexão e
   salvar**. Redes abertas podem deixar a senha vazia.
4. O JrBot testa a autenticação e espera obter IP antes de gravar o perfil.
   Senha incorreta, rede não encontrada ou falta de IP deixam a configuração
   anterior intacta e mantêm a rede de configuração aberta para tentar de novo.
   No sucesso, a tela mostra o IP, o JrBot salva a rede e reinicia após cerca
   de 10 segundos. Reconecte o celular à rede de casa e abra
   `https://jrbot.local` na mesma rede local. O mDNS precisa de multicast
   permitido no roteador e no cliente; o IP atribuído continua disponível
   para diagnóstico. Sem material HTTPS local, teste por HTTP e não considere
   a Live validada.

No painel USB, **Consultar redes salvas** seleciona automaticamente a única
rede da lista. **Excluir rede salva** também aceita a rede selecionada: o
firmware apaga seu perfil e limpa a seleção de boot, mantendo a conexão atual
até reiniciar. No próximo boot, `JrBot-Setup-XXXXXX` fica disponível para
configurar outra rede. As demais redes salvas permanecem no catálogo.

O formulário usa o registro e os perfis Wi-Fi já existentes no JrBot.
No modo normal, o firmware anuncia sempre `jrbot.local`, independentemente
do nome DHCP configurado, e repete a inicialização mDNS após falhas.
O modo de configuração usa AP e um servidor HTTP pequeno. O portal normal,
HTTPS e a Live não são iniciados nesse modo. Depois do reinício com Wi-Fi
conectado, o AP de configuração não fica ativo. No modo de configuração, o
servidor DNS responde com o IP do JrBot e as sondas HTTP do celular levam à
página. A abertura automática depende do sistema operacional e de como ele
faz a detecção; o IP manual continua disponível. Em `_PROV_05`, o arquivo
HTML de provisionamento do branch `test/esp-wifi-provisioning` foi incorporado
e adaptado às rotas e aos perfis do firmware RAM-light. Em `_PROV_06`, a tela
recebeu as cores do painel JrBot e uma sequência explícita: localizar rede,
inserir senha, testar e salvar, consultar redes salvas. O experimento original
tem prioridades e habilitação de até oito redes; o firmware integrado mantém
o limite de quatro perfis e não usa prioridades. A busca é feita sob demanda;
o modo AP+STA existe apenas durante o provisionamento. Se a busca falhar,
o SSID pode ser digitado manualmente.
Como o AP é aberto, pessoas próximas podem acessar a página durante o
provisionamento; encerre esse modo após configurar a rede.

## Medição

Com ESP-IDF 5.5.5 e o mesmo `sdkconfig.develop_wifi_ram_light`, os builds
locais com o inventário de RAM incorporado produziram:

| Recurso reservado no ELF | `_04` | `_PROV_01` | `_PROV_02` | `_PROV_03` | `_PROV_04` | `_PROV_05` | `_PROV_06` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| DRAM | 61.376 B | 61.496 B | 61.520 B | 61.520 B | 61.520 B | 61.520 B | 61.568 B |
| IRAM | 119.808 B | 120.064 B | 120.064 B | 120.064 B | 120.064 B | 120.064 B | 120.064 B |
| Binário de aplicação na flash | 2.542.512 B | 2.549.440 B | 2.550.896 B | 2.550.960 B | 2.557.712 B | 2.565.568 B | 2.569.248 B |

O portal cativo acrescentou 24 B de DRAM estática, 0 B de IRAM e 1.456 B
de flash em relação a `_PROV_01`. O socket DNS usa RAM dinâmica somente
enquanto o AP de configuração está ativo.
O ajuste para excluir a rede selecionada em `_PROV_03` não alterou DRAM ou
IRAM estática e acrescentou 64 B ao binário.
Em `_PROV_04`, a página incorporada e a busca sob demanda acrescentaram
6.752 B de flash e não alteraram DRAM ou IRAM estática em relação a
`_PROV_03`. O modo AP+STA pode usar mais RAM dinâmica enquanto a configuração
está aberta; isso precisa ser medido no aparelho.
Em `_PROV_05`, a página original adaptada e a consulta dos perfis acrescentaram
7.856 B de flash em relação a `_PROV_04`, sem alterar DRAM ou IRAM estática.
Em `_PROV_06`, o teste antes da gravação e o novo layout acrescentaram 48 B
de DRAM estática e 3.680 B de flash em relação a `_PROV_05`; a IRAM não mudou.
O build local de `JrBot_V1.8.1S` com ESP-IDF 5.5.5 e o mesmo perfil mediu
61.576 B de DRAM estática, 120.064 B de IRAM e 2.569.376 B no binário com
inventário incorporado. São mais 8 B de DRAM e 128 B de flash que `_PROV_06`
pela recuperação mDNS e identificação da versão.

Esse inventário mede memória estática. Não mede a alocação dinâmica do
driver Wi-Fi e do servidor HTTP enquanto o AP está aberto. O AP não roda
junto com a Live; no modo normal, só permanece o pequeno custo estático
acima. The historical measurements above are a **static build inventory**, not the dynamic heap profile. On October 26, 2026, the project owner separately confirmed that the physical tests were OK and the memory testing was complete for the current milestone; no additional measurements were requested. No new heap figures are implied by this note.

O log `JR_WIFI_SETUP_MEMORY` informa RAM interna livre, maior bloco e PSRAM
antes e depois de iniciar AP/HTTP. `JR_WIFI_SETUP_READY` indica se o DNS
de portal cativo iniciou. O relatório `JR_MEMORY` no modo normal
permite comparar a Live com os relatórios do firmware `_04`.

Um build local sem `jr_https_material_local.h` desativa HTTPS/WebRTC.
Esse build serve para conferir compilação e memória estática, mas não deve
ser usado para validar a Live. Historically, release acceptance required a build with the local HTTPS material and physical checks for AP, credential saving, reconnection, BOOT recovery and Live. On October 26, 2026, the owner confirmed that physical testing is OK and memory testing is complete for V1.8.1S. No new physical test was executed during this documentation update; earlier logs and measurements retain their original meaning.

## Status de encerramento — 26/10/2026

- Configuração de Wi-Fi pelo celular e mDNS foram integradas na V1.8.1S e permanecem na `JrBot_V1.8.2S` em `main`.
- Testes físicos: **OK**, conforme confirmação explícita do responsável pelo projeto em 26/10/2026.
- Testes de memória: **concluídos**, conforme a mesma confirmação. Não solicitar medições adicionais para encerrar esta etapa.
- A Issue [#55](https://github.com/JuniorNarciso26/JrRobot/issues/55) registra a trilha experimental; a implementação final integrada usa o gerenciador Wi-Fi RAM-light do JrBot e não importa os componentes ADF integralmente.
- Limitações gerais de acesso por Internet e qualidade acústica pertencem a trabalhos separados; não bloqueiam o encerramento do escopo de provisionamento local.
