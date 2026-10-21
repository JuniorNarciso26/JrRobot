# Diagnóstico de memória — develop_wifi STEP03C

No painel do computador e em `https://jrbot.local`, abra **Memória**.
No computador, conecte primeiro pela aba Controle (Serial ou Wi-Fi).
É necessário instalar o firmware STEP03C para obter as leituras.

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
de transporte. Não equivale a um perfil de alocações por módulo ou tarefa.

Esta etapa só adiciona diagnóstico: não reduz pilhas nem altera áudio,
captura da câmera ou parâmetros WebRTC. Compile/instale pelo instalador
do branch develop_wifi e valide na placa antes de concluir sobre estabilidade.
