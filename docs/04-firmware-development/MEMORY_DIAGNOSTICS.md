# Diagnóstico de memória — develop_wifi STEP03E_DIAG

No painel do computador e em `https://jrbot.local`, abra **Memória**.
No computador, conecte primeiro pela aba Controle (Serial ou Wi-Fi).
É necessário instalar o firmware STEP03D para obter a lista de tarefas.
Leituras básicas de heap continuam compatíveis com STEP03C.

## Inventário dos 512 KiB — STEP03E_DIAG

Esta é uma versão temporária de diagnóstico para ESP-IDF 5.5.5/ESP32-S3.
Ela habilita `CONFIG_HEAP_TASK_TRACKING` e `CONFIG_HEAP_TRACK_DELETED_TASKS`
desde o boot. Esses recursos aumentam o custo de RAM/IRAM e o tempo de
alocação/liberação, mesmo quando a tela está fechada. Não comparar o consumo
absoluto desta instrumentação com STEP03D como se o firmware fosse igual.
Após identificar os consumidores, remover a instrumentação da versão normal.

1. Atualize e instale pelo `INSTALAR.bat` no develop_wifi.
2. Abra Memória e clique **Coletar inventário completo**.
3. A coleta funciona em jrbot.local e no painel do computador. O computador
   usa o IPv4 atual do status (ou o campo IP), inclusive no modo Serial.
4. A placa precisa estar acessível por Wi-Fi. As consultas básicas e tarefas
   continuam disponíveis pela Serial; o inventário completo usa HTTP/HTTPS.
5. Clique **Baixar relatório** e guarde o JSON antes e durante a live.

### Conta física, sem duplicar DMA, aliases IRAM/DRAM ou RAM RTC

`GET /memory?full=1` devolve JSON com:

- Cache de instruções e cache de dados: configuração compilada.
- IRAM reservada ao firmware: `_iram_end - _iram_start`, incluindo vetores,
  código e dados/padding dessa área.
- DRAM estática/reservada: `_heap_start - _data_start`, incluindo dados, BSS,
  noinit e alinhamentos; os subtotais são informativos e não somados novamente.
- Heap da SRAM principal: extensão dos heaps registrados, blocos ocupados,
  blocos livres e administração/intervalos calculados pela diferença.
- Parcela sem classificação: diferença assinada para 524288 bytes. Não é
  rotulada como aplicação, vazamento, memória livre ou reserva recuperável.
- Regiões físicas dos heaps internos, com endereços, contagens e maior bloco.
- RAM RTC em uma conta separada dos 512 KiB principais.

As consultas anteriores de `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT` podem incluir
a RAM RTC habilitada como heap, além da SRAM principal. Portanto, não subtrair
diretamente esse total dos 512 KiB. DMA também não é outra memória: é uma
capacidade de uma parte dos heaps e suas linhas se sobrepõem à SRAM.

O visitante oficial `heap_caps_walk` percorre cada heap sob sua própria trava.
O callback só soma números em um workspace de PSRAM; não aloca, não imprime e
não lê conteúdo dos blocos da aplicação. Endereços IRAM e DRAM são aliases
do mesmo SRAM quando usados pelo código; a seção `.dram0.dummy` não é somada
no relatório de compilação. Regiões desconhecidas e falta de capacidade são
expostas, não forçadas a uma categoria. Balanço negativo deve ser investigado.

### Solicitantes, blocos e limites

O inventário usa `heap_caps_get_all_task_stat` para copiar nomes seguros,
solicitantes ativos/encerrados, heaps, bytes solicitados, picos e blocos atuais.
Os nomes repetidos de servidores HTTP/HTTPS são distinguidos pelo handle.
As reservas anteriores ao escalonador ficam em `Pre-scheduler`.

Uma tarefa criadora pode alocar a pilha/TCB de outra tarefa; isso aparece como
reserva da criadora. A tabela não atribui responsabilidade funcional nem
aponta automaticamente um vazamento. Bytes solicitados também diferem de
bytes ocupados nos heaps devido a arredondamento e metadados do alocador e
do rastreamento. Metadados internos do rastreador não são todos atribuídos
como alocações de usuário. Essas diferenças permanecem explícitas.

A coleta é sequencial, com o robô funcionando, e não um congelamento de todo
o sistema. O workspace e o JSON são alocados exclusivamente na PSRAM e o
tamanho desse custo temporário é reportado. Seus blocos podem aparecer no
solicitante que atende a coleta, embora sejam liberados ao terminar a resposta.
Limites: 64 solicitantes, 256 entradas de heap, 4096 blocos, 32 regiões e
256 KiB de JSON. Se os limites forem atingidos, o relatório informa coleta
incompleta ou erro; não afirmar que todos os blocos foram capturados.

As listas de blocos e símbolos mostram inicialmente os 100 maiores e têm
opção de exibir todos os coletados. O download contém os registros completos,
independentemente do limite visual.

### Código e variáveis fixas do ELF/map

O CMake cria um placeholder de inventário no diretório de build e o incorpora
na flash. Após a primeira compilação, `tools/build_memory_report.py` lê o ELF32
e o map produzidos pelo linker, sem depender de ferramentas Python externas.
Gera `jr_memory_build.json` no diretório ignorado de build, religa o firmware
e confirma que seções, símbolos e contribuições de RAM continuam iguais.
Há no máximo três religações; se o inventário não estabilizar, a gravação é
interrompida. Nenhum arquivo gerado é escrito no código-fonte rastreado.

`GET /memory?build=1` serve esse JSON residente na flash. Inclui seções de RAM,
símbolos de funções/variáveis e contribuições por arquivo/biblioteca. O painel
mostra a SRAM principal e valida versão e tamanhos reservados com o inventário
runtime. RTC e PSRAM estáticas ficam categorizadas separadamente no JSON.
Código/constantes mantidos na flash são excluídos; não estão carregados como
reservas permanentes dos 512 KiB. O cache utilizado por eles já é contabilizado.

Somente contribuições efetivas da parte “Linker script and memory map” são
usadas; entradas descartadas e aliases não são somados novamente. Símbolos
podem ter aliases ou sobreposição, portanto sua lista não deve ser somada.
Seções sem atribuição ao objeto de origem continuam explicitamente sem nome.
As identidades de arquivos/bibliotecas não incluem diretórios pessoais do PC.

Em compilação manual, após o primeiro build, execute dentro de firmware:

```sh
python ../tools/build_memory_report.py --build-dir build-develop-wifi-baseline --sdkconfig sdkconfig.develop_wifi --rebuild
```

Sem essa etapa, o firmware informa inventário de compilação ausente ou antigo.

1. Antes da live, selecione **Antes da live** e clique **Salvar leitura**.
2. Volte a Controle e inicie a live em 320×240.
3. Abra Memória, selecione **Durante a live** e salve outra leitura.
4. Pare a live em Controle e salve uma leitura **Depois da live**.
5. Clique **Baixar relatório** para exportar as últimas 60 leituras em JSON.

Atualização automática é opcional, a cada cinco segundos. Só consulta com
a aba Memória aberta e a página visível. Após erro, exige reativação manual.
As leituras ficam no navegador e são perdidas ao recarregar a página.

O comando Serial `memoria` e a rota HTTP/HTTPS `GET /memory` retornam
`JR_MEMORY protocol=1`, tempo ligado em milissegundos e, para RAM interna,
DMA interna e PSRAM, bytes livres, alocados no heap, maior bloco livre e
mínimo livre histórico. A tabela exibe KiB (1024 bytes). A consulta não
inicializa câmera ou áudio e não cria tarefa ou buffer persistente.

DMA é um subconjunto da RAM interna: não somar as linhas. Alocações do heap
não contabilizam todos os usos da RAM física. O mínimo livre agrega mínimos
dos heaps desde o boot; não descreve um único instante ou uma sessão da live.
A consulta usa a pilha existente do servidor/terminal e tem custo transitório
de transporte. Não equivale a um perfil completo de alocações por módulo.

## Tarefas e pilhas

Clique **Consultar tarefas**. Essa consulta é separada da atualização automática.
O comando Serial `tarefas` e `GET /memory?tasks=1` retornam `JR_TASKS` seguido de
JSON v1. As linhas compactas têm: ID RTOS, ID do catálogo, estado RTOS,
prioridade, menor sobra de pilha em bytes, tamanho configurado conhecido
(0 = não informado) e localização da pilha (0 = não identificada, 1 = interna,
2 = PSRAM). O relatório exportado inclui a última coleta de tarefas e os
nomes/funções interpretados pelo painel.

A coleta usa `uxTaskGetSystemState`, com limite de 48 tarefas e espaço temporário
exclusivamente em PSRAM, liberado antes de devolver a resposta. A varredura
pode pausar brevemente o escalonamento; a duração é mostrada em milissegundos.
Se houver excesso de tarefas, falta de PSRAM ou resposta grande demais, retorna
erro explícito sem publicar uma lista incompleta. Não ativa estatísticas de CPU,
perfil de heap por tarefa ou formatação de estatísticas do FreeRTOS.

O instalador garante `CONFIG_FREERTOS_USE_TRACE_FACILITY=y` no sdkconfig existente
apenas em develop_wifi; projetos novos usam sdkconfig.defaults. Isso acrescenta
metadados de rastreamento aos controles das tarefas, portanto o diagnóstico tem
um pequeno custo de RAM. Compilações manuais com trace desativado retornam um
erro indicando que é necessário atualizar a configuração/reinstalar.

Os ponteiros de nomes retornados pelo RTOS podem expirar quando uma tarefa é
excluída. O diagnóstico não os desreferencia: associa handles a um catálogo
constante por meio de `xTaskGetHandle`. Tarefas desconhecidas, encerrando ou com
nomes repetidos podem aparecer apenas com seu ID. São identificações aproximadas
de um sistema que continua executando; não há soma confiável de todas as pilhas.

A pilha configurada é uma referência do código, não o tamanho de alocação medido.
A sobra é o mínimo histórico da pilha e inclui a execução do próprio diagnóstico
na tarefa que o atende. Não reduzir uma pilha apenas com uma consulta: exercitar
os caminhos de câmera, rede, áudio e erros. Filas, buffers e alocações de cada
tarefa não estão incluídos nessa coluna. Depois de parar a live, uma redução
persistente da memória livre não prova vazamento: buffers da câmera e outros
recursos podem continuar inicializados.

## Validação local

Sem ESP-IDF/placa, os testes usam interfaces simuladas e verificam os contratos,
limites, liberação do espaço temporário e o fluxo dos dois painéis. Não substituem
a compilação com ESP-IDF nem o teste físico. Executar a partir da raiz:

```sh
node tests/memory/memory-panel.test.cjs
python tests/memory/test_memory_transport.py
python tests/memory/test_build_inventory.py
python tests/memory/test_full_report.py
cc -std=c11 -Wall -Wextra -Werror -Itests/memory -Ifirmware/core tests/memory/memory_tasks_test.c firmware/core/jr_memory_tasks.c -o /tmp/jr-memory-tasks-test
/tmp/jr-memory-tasks-test
```

O teste `test_full_report.py` compila fixtures de host Linux com ASan/UBSan,
verifica o balanço físico, exclusão da RTC, JSON e limpeza de alocações em
falhas. O teste do ELF/map usa um ELF32 sintético e verifica entradas
descartadas, aliases e a validação de religações. Esses resultados não
confirmam uma compilação ESP-IDF 5.5.5 nem execução real na placa.

Esta etapa só adiciona diagnóstico: não reduz pilhas nem altera áudio,
captura da câmera ou parâmetros WebRTC. Compile/instale pelo instalador
do branch develop_wifi e valide na placa antes de concluir sobre estabilidade.
