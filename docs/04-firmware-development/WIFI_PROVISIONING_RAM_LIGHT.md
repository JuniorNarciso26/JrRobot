# Provisionamento de Wi-Fi do JrBot V1.8.1S

Versão atual em `main`: `JrBot_V1.8.1S`. O desenvolvimento passou pelo branch
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
acima. A RAM livre real precisa ser confirmada na ESP32.

O log `JR_WIFI_SETUP_MEMORY` informa RAM interna livre, maior bloco e PSRAM
antes e depois de iniciar AP/HTTP. `JR_WIFI_SETUP_READY` indica se o DNS
de portal cativo iniciou. O relatório `JR_MEMORY` no modo normal
permite comparar a Live com os relatórios do firmware `_04`.

Um build local sem `jr_https_material_local.h` desativa HTTPS/WebRTC.
Esse build serve para conferir compilação e memória estática, mas não deve
ser usado para validar a Live. O teste físico precisa do material HTTPS
local e deve confirmar AP, gravação, reconexão, recuperação por BOOT e Live.
