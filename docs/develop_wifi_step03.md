# develop_wifi — STEP03: nome local com DHCP

Versao: `JrBot_V1S_WIFI_STEP03` (2026-10-20).
Base: STEP02A, commit `1a3d5ca2ea0f0804de71a51e8256d58d42fde087`.

## Escopo

Adicionar somente a descoberta mDNS para abrir `https://jrbot.local` na mesma
rede local, com o endereco atribuido por DHCP ou IP fixo. O nome vem do hostname
ja configurado na estacao: se for alterado para `my-robot`, o acesso sera
`https://my-robot.local` depois de reiniciar.

Dependencia adicionada: `espressif/mdns` fixada em `1.14.0`. Um servico
`_https._tcp` na porta 443, uma interface STA, sem browse continuo, console mDNS,
interface AP/Ethernet ou backend de sockets BSD. Buffers em PSRAM, stack interna
de 4096 bytes. Nao altera os ajustes de WebRTC, TLS, sockets, camera ou audio.
Nao inclui o gerenciador Wi-Fi/ADF da antiga integracao no develop.

O loop principal existente chama o inicializador uma unica vez, depois de
receber IP e encontrar o modo idle. A inicializacao nao usa a stack da tarefa
de eventos da rede. Habilita e anuncia explicitamente o IPv4 ja obtido; os
handlers do componente oficial acompanham desconexoes e mudancas de IP futuras.
Falha de mDNS e registrada e limpa, sem impedir o acesso por IP; nao tenta
reinicializar continuamente durante a operacao.

`sdkconfig.defaults` define o perfil mDNS; o instalador aplica o mesmo perfil
ao `sdkconfig.develop_wifi` existente antes de compilar. O codigo recusa a
compilacao se os buffers nao estiverem em PSRAM ou a stack estiver externa.

## Registros

- `JR_MDNS_CONFIG`: politica de memoria e configuracao efetiva de stack,
  interfaces, servicos, stack de eventos e limite de sockets.
- `JR_MDNS_MEMORY stage=before_init/after_init`: RAM interna livre, maior bloco,
  memoria DMA livre/maior bloco e PSRAM livre. Em falha, `after_failure`.
- `JR_MDNS_READY`: URL e IP corrente. Indica inicializacao/anuncio enfileirado,
  nao comprova resolucao do nome em outro dispositivo.
- `JR_MDNS_ERROR`: etapa e retorno de erro; acesso por IP permanece disponivel.
- Os registros `HEAP` da Live permanecem para comparar com a base aprovada.

Referencia fisica mais recente: log 22 executava STEP02, e nao STEP02A. Busca
de cinco redes terminou com 127635 bytes internos livres e maior bloco 53248,
iguais antes/depois. Capturou fotos VGA/XGA; a Live conectou e enviou 50 quadros,
com dois descartes de buffer no inicio. Antes do peer havia 74843 bytes internos
livres/maior bloco 31744; no canal de video, 23471/11264. Sao pontos de referencia,
nao limites de aprovacao. A STEP03 inclui tambem as correcoes ja publicadas na
STEP02A (busca durante reinicio pendente e selecao da mesma configuracao).

## Teste na placa

1. Instalar a branch `develop_wifi` e confirmar `JrBot_V1S_WIFI_STEP03`.
2. Conectar Serial desde o inicio e procurar `JR_MDNS_CONFIG`, os dois registros
   de memoria e `JR_MDNS_READY url=https://jrbot.local`.
3. No computador/celular na mesma rede, abrir `https://jrbot.local`.
4. Capturar foto. Abrir a Live, testar video e audio nos dois sentidos, parar e
   abrir novamente. Comparar os registros de memoria com o teste anterior.
5. Reiniciar o robo e acessar novamente pelo mesmo nome, sem consultar o IP.
6. Repetir busca de redes apos selecionar a configuracao atual (correcao STEP02A).
7. Baixar o log desde o boot. Se o nome falhar, testar tambem pelo IP registrado
   para distinguir descoberta de nome e funcionamento do portal/Live.

O navegador pode continuar apresentando o aviso do certificado HTTPS local do
firmware; esta etapa nao altera certificados. Resolucao multicast depende da
rede e do suporte do dispositivo cliente.

## Verificacao local

`python tests/mdns/test_mdns.py`: compila o wrapper C real com APIs simuladas,
testa inicializacao depois de IP/idle, anuncio HTTPS, hostname configuravel,
inicializacao unica, liberacao em cada etapa de falha e rejeicao de configuracao
de memoria incorreta. Verifica migracao idempotente do sdkconfig preservando
ajustes de Live/TLS/sockets e consistencia com defaults.

Compilacao ESP-IDF completa, consumo real de memoria, resolucao mDNS na rede e
Live no hardware dependem do instalador e da placa do usuario.

Fontes oficiais consultadas:

- [Espressif mDNS 1.14.0](https://components.espressif.com/components/espressif/mdns/versions/1.14.0)
- [Kconfig da versao fixada](https://github.com/espressif/esp-protocols/blob/mdns-v1.14.0/components/mdns/Kconfig)
- [Eventos e interfaces](https://github.com/espressif/esp-protocols/blob/mdns-v1.14.0/components/mdns/mdns_netif.c)
