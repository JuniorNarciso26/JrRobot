# Diagnóstico de memória — develop_wifi STEP03D

No painel do computador e em `https://jrbot.local`, abra **Memória**.
No computador, conecte primeiro pela aba Controle (Serial ou Wi-Fi).
É necessário instalar o firmware STEP03D para obter a lista de tarefas.
Leituras básicas de heap continuam compatíveis com STEP03C.

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
cc -std=c11 -Wall -Wextra -Werror -Itests/memory -Ifirmware/core tests/memory/memory_tasks_test.c firmware/core/jr_memory_tasks.c -o /tmp/jr-memory-tasks-test
/tmp/jr-memory-tasks-test
```

Esta etapa só adiciona diagnóstico: não reduz pilhas nem altera áudio,
captura da câmera ou parâmetros WebRTC. Compile/instale pelo instalador
do branch develop_wifi e valide na placa antes de concluir sobre estabilidade.
