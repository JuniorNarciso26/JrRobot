(function () {
  'use strict';
  const main = document.querySelector('main');
  if (!main || document.getElementById('jr-memory-panel')) return;
  const desktop = !!document.getElementById('esp_ip');
  const style = document.createElement('style');
  style.textContent = '[data-jr-memory-main][hidden],#jr-memory-panel[hidden]{display:none!important}.jr-memory-nav{display:flex;gap:8px;max-width:820px;margin:12px auto;padding:0 14px}.jr-memory-nav button[aria-pressed=true]{border-color:#7394ff;background:#29415f}#jr-memory-panel{max-width:1000px;margin:14px auto;padding:16px;color:inherit}#jr-memory-panel .memory-scroll{overflow-x:auto}#jr-memory-panel table{width:100%;border-collapse:collapse;margin:14px 0}#jr-memory-panel th,#jr-memory-panel td{text-align:left;padding:10px;border-bottom:1px solid #29415f;white-space:nowrap}#jr-memory-panel .memory-tools{display:flex;flex-wrap:wrap;align-items:center;gap:10px;margin:12px 0}#jr-memory-panel p{line-height:1.5}#jr-memory-message{min-height:24px}';
  document.head.appendChild(style);
  const nav = document.createElement('nav');
  nav.className = 'jr-memory-nav';
  nav.setAttribute('aria-label', 'Seções do painel');
  nav.innerHTML = '<button type="button" aria-pressed="true">Controle</button><button type="button" aria-pressed="false">Memória</button>';
  const panel = document.createElement('section');
  panel.id = 'jr-memory-panel';
  panel.className = 'card';
  panel.hidden = true;
  panel.innerHTML = '<h2>Memória do JrBot</h2>' +
    '<p>Consulte antes, durante e depois da live. DMA é uma parte da RAM interna: não some as duas linhas.</p>' +
    '<div class="memory-tools"><button type="button" id="jr-memory-refresh">Consultar memória</button><label><input type="checkbox" id="jr-memory-auto"> Atualizar a cada 5 segundos</label></div>' +
    '<div id="jr-memory-message" role="status">Aguardando consulta.</div>' +
    '<div class="memory-scroll"><table><thead><tr><th>Memória</th><th>Livre agora</th><th>Alocada no heap</th><th>Maior bloco livre</th><th>Mínimo livre desde o boot</th></tr></thead><tbody id="jr-memory-values"></tbody></table></div>' +
    '<p>Valores em KiB (1 KiB = 1.024 bytes). Alocada no heap não representa todo o uso físico dos 512 KB. O mínimo é histórico e pode continuar baixo depois de parar a live. Estas leituras não detalham o consumo por tarefa.</p>' +
    '<p id="jr-memory-accounting"></p><p id="jr-memory-blocks"></p>' +
    '<h3>Comparação das etapas</h3><p id="jr-memory-comparison">Salve uma leitura Antes da live e outra Durante ou Depois para comparar.</p>' +
    '<div class="memory-tools"><label>Marcar leitura <select id="jr-memory-stage"><option value="antes">Antes da live</option><option value="durante">Durante a live</option><option value="depois">Depois da live</option></select></label><button type="button" id="jr-memory-save">Salvar leitura</button><button type="button" id="jr-memory-download">Baixar relatório</button></div>' +
    '<p>As últimas 60 leituras ficam neste navegador até recarregar a página. Baixe o relatório para enviar à análise.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Horário</th><th>Etapa</th><th>RAM livre</th><th>DMA livre</th><th>PSRAM livre</th></tr></thead><tbody id="jr-memory-history"></tbody></table></div>' +
    '<h3>Tarefas em execução</h3><div class="memory-tools"><button type="button" id="jr-memory-tasks-refresh">Consultar tarefas</button></div>' +
    '<p id="jr-memory-tasks-message" role="status">Consulta manual: a varredura das pilhas pode causar uma pequena pausa.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Tarefa / ID</th><th>Função</th><th>Estado</th><th>Prioridade</th><th>Pilha configurada*</th><th>Menor sobra de pilha</th><th>Memória da pilha</th></tr></thead><tbody id="jr-memory-tasks"></tbody></table></div>' +
    '<p>* Tamanho configurado no código, quando conhecido. A menor sobra é medida desde a criação da tarefa. Pilhas não incluem filas, buffers, conexões ou outras alocações da tarefa. Uma tarefa bloqueada está aguardando trabalho, mas mantém sua pilha reservada.</p>' +
    '<p>O RTOS identifica tarefas conhecidas pelo nome. Nomes repetidos, tarefas criadas durante a coleta e tarefas de bibliotecas sem identificação aparecem como Tarefa #ID. Não é uma lista de programas independentes nem de todo o consumo de RAM.</p>' +
    '<h3>Inventário dos 512 KiB e responsáveis pelas reservas</h3>' +
    '<div class="memory-tools"><button type="button" id="jr-memory-full-refresh">Coletar inventário completo</button><label>Etapa <select id="jr-memory-full-stage"><option value="consulta">Consulta</option><option value="antes">Antes da live</option><option value="durante">Durante a live</option><option value="depois">Depois da live</option></select></label></div>' +
    '<p>Esta versão é de diagnóstico: rastreia alocações desde o boot e tem custo de RAM e processamento. A coleta é manual e usa Wi-Fi mesmo quando o painel está conectado pela Serial.</p>' +
    '<p id="jr-memory-full-message" role="status">Instale STEP03F_DIAG pelo instalador para incorporar também o inventário do arquivo compilado.</p>' +
    '<h4>Distribuição dos 512 KiB principais</h4><div class="memory-scroll"><table><thead><tr><th>Área</th><th>Tamanho</th><th>Origem da informação</th></tr></thead><tbody id="jr-memory-layout"></tbody></table></div>' +
    '<p id="jr-memory-layout-note"></p>' +
    '<h4>Regiões e reservas dentro de outros heaps</h4><p>Uma reserva pode conter outro heap. A distribuição física desconta essa sobreposição; os valores originais ficam disponíveis abaixo.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Região</th><th>Tipo</th><th>Extensão</th><th>Ocupação original</th><th>Parte que contém outro heap</th><th>Livre</th><th>Maior bloco</th></tr></thead><tbody id="jr-memory-regions"></tbody></table></div>' +
    '<h4>Quem solicitou as reservas do heap</h4><p>Inclui buffers, filas, conexões, pilhas e outras alocações. Uma tarefa pode reservar a pilha de outra: o solicitante não é necessariamente quem usa o bloco. Não some esta tabela à de pilhas ou à distribuição dos 512 KiB.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Solicitante / handle</th><th>Situação</th><th>SRAM solicitada original</th><th>Contém outro heap*</th><th>Parte em heap DMA*</th><th>RAM RTC</th><th>PSRAM</th><th>Blocos atuais</th></tr></thead><tbody id="jr-memory-owners"></tbody></table></div>' +
    '<p id="jr-memory-owner-note"></p>' +
    '<h4>100 maiores grupos de blocos repetidos</h4><p>Agrupados por solicitante, tipo de memória e tamanho. Ajuda a localizar conjuntos de buffers; tamanhos iguais não provam a mesma finalidade.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Solicitante</th><th>Memória</th><th>Tamanho por bloco</th><th>Quantidade</th><th>Total registrado*</th></tr></thead><tbody id="jr-memory-block-groups"></tbody></table></div>' +
    '<h4>Configuração compilada de buffers e reservas</h4><p>São quantidades e limites configurados, não consumo medido. Não some esses valores às tabelas de RAM.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Parâmetro</th><th>Valor compilado</th></tr></thead><tbody id="jr-memory-allocation-config"></tbody></table></div>' +
    '<h4>Comparação dos inventários completos</h4><p id="jr-memory-full-comparison">Colete Antes da live e depois Durante ou Depois. Até 6 inventários ficam neste navegador até recarregar; o download inclui todos.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Solicitante</th><th>Blocos novos / bytes</th><th>Blocos que desapareceram / bytes</th><th>Variação SRAM solicitada*</th><th>Variação PSRAM</th></tr></thead><tbody id="jr-memory-deltas"></tbody></table></div>' +
    '<h4>Blocos individuais atualmente reservados</h4><div class="memory-tools"><label>Exibir <select id="jr-memory-block-limit"><option value="100">100 maiores</option><option value="all">Todos os coletados</option></select></label></div>' +
    '<p>Finalidade dos blocos: só identificamos automaticamente reservas que contêm outro heap. Nos demais, o solicitante é conhecido, mas a função do buffer ainda não foi instrumentada.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Solicitante</th><th>Memória</th><th>Endereço</th><th>Tamanho registrado</th><th>Finalidade identificada</th></tr></thead><tbody id="jr-memory-block-list"></tbody></table></div>' +
    '<h4>Código e variáveis fixas carregados na RAM</h4><p id="jr-memory-build-message"></p>' +
    '<p>Código: instruções executadas na RAM. Dados: variáveis inicializadas. BSS: variáveis com reserva fixa, inicialmente zeradas. Estes valores já fazem parte da distribuição física; não some novamente.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Biblioteca / componente</th><th>Total RAM</th><th>Código</th><th>Dados</th><th>BSS</th></tr></thead><tbody id="jr-memory-components"></tbody></table></div>' +
    '<h4>100 arquivos que mais ocupam RAM fixa</h4><div class="memory-scroll"><table><thead><tr><th>Arquivo / biblioteca</th><th>Total RAM</th><th>Código</th><th>Dados</th><th>BSS</th></tr></thead><tbody id="jr-memory-objects"></tbody></table></div>' +
    '<div class="memory-tools"><label>Símbolos <select id="jr-memory-symbol-limit"><option value="100">100 maiores</option><option value="all">Todos os coletados</option></select></label></div>' +
    '<div class="memory-scroll"><table><thead><tr><th>Função ou variável</th><th>RAM</th><th>Seção</th><th>Origem</th></tr></thead><tbody id="jr-memory-symbols"></tbody></table></div>' +
    '<p>O relatório para baixar inclui todos os blocos e símbolos coletados, mesmo quando a tela mostra apenas os 100 maiores. Parcela sem classificação, lacunas ou limites de coleta não são tratados como consumo identificado.</p>';
  main.setAttribute('data-jr-memory-main', '');
  main.before(nav, panel);
  const get = id => document.getElementById('jr-memory-' + id);
  const controls = nav.querySelectorAll('button');
  const history = [];
  let active = false, reading = false, timer = null, generation = 0;
  let tasksReading = false, taskSnapshot = null;
  let fullReading = false, fullSnapshot = null, buildInventory = null;
  const fullHistory = [];
  // IDs correspond to the immutable catalog in jr_memory_tasks.c.
  const taskCatalog = [null,
    ['main', 'Laço principal'], ['terminal', 'Comandos Serial'],
    ['usb_terminal', 'Comandos USB nativo'], ['httpd', 'Servidor HTTP ou HTTPS'],
    ['mdns', 'Nome jrbot.local'], ['mdns_task', 'Nome jrbot.local'],
    ['jr_wa_peer', 'Conexão WebRTC'], ['jr_wa_mic', 'Microfone da live'],
    ['jr_wa_play', 'Alto-falante da live'], ['jr_wa_video', 'Envio de vídeo'],
    ['jr_wa_signal', 'Sinalização WebRTC'], ['jr_mic_stream', 'Transmissão do microfone'],
    ['jr_mic_ws', 'Microfone por WebSocket'], ['wifi', 'Wi-Fi'],
    ['tiT', 'Rede TCP/IP'], ['sys_evt', 'Eventos do sistema'],
    ['esp_timer', 'Temporizadores ESP-IDF'], ['Tmr Svc', 'Temporizadores RTOS'],
    ['IDLE0', 'Tarefa ociosa núcleo 0'], ['IDLE1', 'Tarefa ociosa núcleo 1'],
    ['ipc0', 'Comunicação entre núcleos'], ['ipc1', 'Comunicação entre núcleos'],
    ['cam_task', 'Captura da câmera']];
  const kib = bytes => (bytes / 1024).toFixed(1) + ' KiB';

  function parse(text) {
    if (!text.startsWith('JR_MEMORY protocol=1 ')) {
      throw Error('Diagnóstico indisponível. Instale o firmware STEP03C ou posterior.');
    }
    const result = {};
    for (const token of text.trim().split(/\s+/).slice(1)) {
      const [key, value] = token.split('=');
      if (key === 'version') result.version = value;
      if (/^\d+$/.test(value || '')) result[key] = Number(value);
    }
    const required = ['uptime_ms'];
    for (const prefix of ['internal', 'dma', 'psram']) {
      for (const metric of ['free', 'allocated', 'largest', 'min']) required.push(prefix + '_' + metric);
    }
    if (required.some(key => !Number.isSafeInteger(result[key]) || result[key] < 0)) {
      throw Error('Resposta de memória incompleta ou inválida.');
    }
    return result;
  }

  function row(body, cells) {
    const tr = document.createElement('tr');
    for (const text of cells) {
      const td = document.createElement('td');
      td.textContent = text;
      tr.appendChild(td);
    }
    body.appendChild(tr);
  }

  function render(data) {
    get('values').replaceChildren();
    for (const [prefix, label] of [['internal', 'RAM interna (pode incluir RTC)'], ['dma', 'DMA interna'], ['psram', 'PSRAM']]) {
      row(get('values'), [label, ...['free', 'allocated', 'largest', 'min'].map(k => kib(data[prefix + '_' + k]))]);
    }
    get('message').textContent = 'Atualizado às ' + new Date().toLocaleTimeString() +
      ' · placa ligada há ' + Math.floor(data.uptime_ms / 1000) + ' s · ' + (data.version || 'versão não informada') + '.';
    const heap = data.internal_free + data.internal_allocated;
    get('accounting').textContent = 'RAM interna contabilizada nestes heaps: ' + kib(heap) +
      ' (livre + alocada). Isso não equivale aos 512 KiB físicos: código em RAM, dados fixos, reservas do sistema e metadados precisam do relatório de compilação para serem detalhados.';
    get('blocks').textContent = 'Maior bloco DMA disponível: ' + kib(data.dma_largest) +
      '. Uma reserva contínua maior pode falhar mesmo com ' + kib(data.dma_free) + ' livres no total. DMA compartilha memória com a RAM interna.';
  }

  function compare() {
    const latest = stage => [...history].reverse().find(item => item.stage === stage);
    const before = latest('antes');
    const lines = [];
    for (const [stage, label] of [['durante', 'Durante'], ['depois', 'Depois']]) {
      const sample = latest(stage);
      if (!before || !sample || sample.time < before.time || sample.uptime_ms < before.uptime_ms) continue;
      const changes = [['internal', 'RAM'], ['dma', 'DMA'], ['psram', 'PSRAM']].map(([key, name]) => {
        const difference = sample[key + '_free'] - before[key + '_free'];
        return name + ': ' + (difference < 0 ? 'reduziu ' : 'aumentou ') + kib(Math.abs(difference)) + ' livres';
      });
      lines.push(label + ' em relação a Antes — ' + changes.join('; ') + '.');
    }
    get('comparison').textContent = lines.join(' ') || 'Salve Antes da live e depois Durante ou Depois, nesta ordem e sem reiniciar a placa.';
  }

  async function readTasks() {
    if (reading || tasksReading || fullReading || !active || document.hidden) return;
    if (desktop && typeof busy !== 'undefined' && busy) {
      get('tasks-message').textContent = 'Aguarde o comando em andamento e consulte novamente.';
      return;
    }
    tasksReading = true;
    get('tasks-refresh').disabled = true;
    syncTimer();
    const requestGeneration = generation;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 8000);
    try {
      get('tasks-message').textContent = 'Coletando tarefas e menor sobra das pilhas...';
      let text;
      if (desktop) text = await send('tarefas');
      else {
        const response = await fetch('/memory?tasks=1', {cache: 'no-store', signal: controller.signal});
        text = await response.text();
        if (!response.ok) throw Error(text);
      }
      if (requestGeneration !== generation || !active) return;
      if (!text.startsWith('JR_TASKS ')) throw Error('Instale STEP03D para consultar tarefas. ' + text);
      const data = JSON.parse(text.slice(9));
      if (data.v !== 1 || !Array.isArray(data.rows) || data.rows.length > 48 || data.total !== data.rows.length ||
          !Number.isSafeInteger(data.scan_us) || data.scan_us < 0 || !Number.isSafeInteger(data.workspace_bytes) || data.workspace_bytes < 0 ||
          data.rows.some(r => !Array.isArray(r) || r.length !== 7 || r.some(n => !Number.isSafeInteger(n) || n < 0) || r[1] >= taskCatalog.length || r[2] > 5 || r[6] > 2)) {
        throw Error('Resposta de tarefas inválida.');
      }
      const states = ['Executando', 'Pronta', 'Aguardando', 'Suspensa', 'Encerrando', 'Indefinida'];
      const rows = data.rows.map(([id, code, state, priority, minFree, configured, memory]) => ({
        id, name: taskCatalog[code]?.[0] || 'Tarefa', purpose: taskCatalog[code]?.[1] || 'Não identificada',
        state, priority, minFree, configured, memory}));
      rows.sort((a, b) => b.configured - a.configured || b.minFree - a.minFree);
      get('tasks').replaceChildren();
      for (const task of rows) {
        row(get('tasks'), [task.name + ' #' + task.id, task.purpose, states[task.state], String(task.priority),
          task.configured ? kib(task.configured) : 'Não informado', kib(task.minFree),
          ['Não identificada', 'Interna', 'PSRAM'][task.memory]]);
      }
      taskSnapshot = {time: new Date().toISOString(), ...data, tasks: rows};
      get('tasks-message').textContent = rows.length + ' tarefas · atualizado às ' + new Date().toLocaleTimeString() +
        ' · varredura ' + (data.scan_us / 1000).toFixed(2) + ' ms · espaço temporário PSRAM ' + kib(data.workspace_bytes) + ' já liberado.';
    } catch (error) {
      if (requestGeneration === generation && active) get('tasks-message').textContent = 'Erro: ' + error.message + ' A lista anterior pode estar desatualizada.';
    } finally {
      clearTimeout(timeout);
      tasksReading = false;
      get('tasks-refresh').disabled = false;
      syncTimer();
    }
  }

  function syncTimer() {
    if (timer !== null) clearTimeout(timer);
    timer = null;
    if (active && !document.hidden && get('auto').checked && !reading && !tasksReading && !fullReading) {
      timer = setTimeout(() => { timer = null; read(); }, 5000);
    }
  }

  async function read(stage) {
    if (reading || tasksReading || fullReading || !active || document.hidden) return;
    if (desktop && typeof busy !== 'undefined' && busy) {
      get('message').textContent = 'Aguarde o comando em andamento e consulte novamente.';
      syncTimer();
      return;
    }
    reading = true;
    get('refresh').disabled = true;
    get('save').disabled = true;
    const requestGeneration = generation;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 8000);
    try {
      get('message').textContent = 'Consultando memória...';
      let text;
      if (desktop) {
        text = await send('memoria');
      } else {
        const response = await fetch('/memory', {cache: 'no-store', signal: controller.signal});
        if (!response.ok) throw Error(response.status === 404 ? 'Instale o firmware STEP03C ou posterior.' : 'Falha HTTP ' + response.status);
        text = await response.text();
      }
      if (requestGeneration !== generation || !active) return;
      const data = parse(text);
      render(data);
      const sample = {time: new Date().toISOString(), stage: stage || 'consulta', ...data};
      history.push(sample);
      if (history.length > 60) history.shift();
      compare();
      get('history').replaceChildren();
      for (const item of history) {
        row(get('history'), [new Date(item.time).toLocaleTimeString(), item.stage,
          kib(item.internal_free), kib(item.dma_free), kib(item.psram_free)]);
      }
    } catch (error) {
      if (requestGeneration === generation && active) {
        get('message').textContent = 'Erro: ' + (error.name === 'AbortError' ? 'Tempo de consulta esgotado.' : error.message) + ' A tabela mantém a última leitura válida.';
        get('auto').checked = false;
      }
    } finally {
      clearTimeout(timeout);
      reading = false;
      get('refresh').disabled = false;
      get('save').disabled = false;
      syncTimer();
    }
  }

  const hex = address => '0x' + Number(address).toString(16).padStart(8, '0');

  function heapKind(heap, caps) {
    if (heap.caps & caps.psram) return 'PSRAM';
    if (heap.caps & caps.rtc) return 'RTC';
    return heap.caps & caps.internal ? 'SRAM principal' : 'Outra';
  }

  function nestedBytes(data, address, size) {
    const intervals = (data.regions || []).filter(r => r.kind === 1 &&
      r.start >= address && r.end <= address + size).sort((a, b) => a.start - b.start);
    let end = address, bytes = 0;
    for (const r of intervals) { bytes += Math.max(0, r.end - Math.max(end, r.start)); end = Math.max(end, r.end); }
    return bytes;
  }

  function allBlocks(data) {
    const blocks = [];
    for (const task of data.tasks) for (const heap of task.heaps) for (const [address, size] of heap.blocks) {
      blocks.push({owner: task.name + ' ' + hex(task.handle), memory: heapKind(heap, data.caps), address, size});
    }
    return blocks;
  }

  function renderFullComparison() {
    get('deltas').replaceChildren();
    get('full-comparison').textContent = 'Colete Antes da live e depois Durante ou Depois, sem reiniciar. Até 6 inventários ficam no navegador e no download.';
    const current = fullSnapshot;
    const before = [...fullHistory].reverse().find(s => s.stage === 'antes' && s !== current &&
      s.version === current.version && s.uptime_ms <= current.uptime_ms && s.time <= current.time);
    if (!before || !['durante', 'depois'].includes(current.stage)) return;
    if (!before.complete || !current.complete || before.layout.accounting_consistent === false || current.layout.accounting_consistent === false) {
      get('full-comparison').textContent = 'Comparação indisponível: um dos inventários está incompleto ou tem balanço inconsistente.';
      return;
    }
    const key = b => [b.owner, b.memory, b.address, b.size].join('|');
    const old = new Map(allBlocks(before).map(b => [key(b), b]));
    const now = new Map(allBlocks(current).map(b => [key(b), b]));
    const groups = new Map();
    for (const [set, other, sign] of [[now, old, 1], [old, now, -1]]) for (const [id, block] of set) {
      if (other.has(id)) continue;
      if (!groups.has(block.owner)) groups.set(block.owner, {added: 0, addedBytes: 0, removed: 0, removedBytes: 0, main: 0, psram: 0});
      const g = groups.get(block.owner), prefix = sign > 0 ? 'added' : 'removed';
      g[prefix]++; g[prefix + 'Bytes'] += block.size;
      if (block.memory === 'SRAM principal') g.main += sign * block.size;
      if (block.memory === 'PSRAM') g.psram += sign * block.size;
    }
    for (const [owner, g] of [...groups].sort((a, b) => Math.abs(b[1].main) - Math.abs(a[1].main))) {
      row(get('deltas'), [owner, g.added + ' / ' + kib(g.addedBytes), g.removed + ' / ' + kib(g.removedBytes), kib(g.main), kib(g.psram)]);
    }
    get('full-comparison').textContent = current.stage + ' versus antes: RAM física ocupada variou ' +
      kib(current.layout.heap_allocated_blocks - before.layout.heap_allocated_blocks) +
      '. * Variações por solicitante são registros originais, podendo conter reservas de outros heaps. ' +
      'Endereços podem ser reutilizados entre coletas; isto não é histórico de todas as alocações nem prova de vazamento. Reinícios podem invalidar a comparação; colete tudo sem reiniciar.';
  }

  function renderFull(data) {
    const layout = data.layout;
    get('layout').replaceChildren();
    for (const [key, label, source] of [
      ['icache', 'Cache de instruções', 'Configuração compilada'],
      ['dcache', 'Cache de dados', 'Configuração compilada'],
      ['iram_image', 'Código, vetores e dados fixos em IRAM', 'Limites do linker'],
      ['dram_image', 'Variáveis fixas e reserva estática em DRAM', 'Limites do linker'],
      ['heap_allocated_blocks', 'Blocos atualmente ocupados no heap principal', 'Varredura do alocador'],
      ['heap_free_blocks', 'Blocos livres no heap principal', 'Varredura do alocador'],
      ['heap_admin', 'Administração e intervalos internos dos heaps', 'Extensão dos heaps menos seus blocos'],
      ['unclassified', 'Parcela sem classificação detalhada', 'Diferença para os 512 KiB físicos']
    ]) row(get('layout'), [label, kib(layout[key]), source]);
    row(get('layout'), ['Total físico principal', kib(layout.physical_sram), 'ESP32-S3']);
    get('layout-note').textContent = 'Fora dessa soma: heap da RAM RTC ' + kib(layout.rtc_heap_extent) +
      ' e PSRAM externa. Dentro da reserva DRAM: dados ' + kib(layout.data) + ', BSS ' + kib(layout.bss) + ', noinit ' + kib(layout.noinit) +
      '. Esses subtotais não devem ser somados novamente. ' +
      ((layout.unclassified < 0 || layout.heap_admin < 0 || layout.region_overflow || layout.accounting_consistent === false) ? 'ATENÇÃO: coleta de regiões incompleta ou balanço inconsistente; não considerar a conta fechada.' :
      layout.unclassified > 0 ? 'A parcela sem classificação não foi atribuída a uma aplicação nem declarada livre.' : 'As áreas contabilizadas fecham os 512 KiB nesta coleta.');
    if (layout.heap_nested_bytes) get('layout-note').textContent += ' Sobreposição descontada: ' + kib(layout.heap_nested_bytes) + '. Isso corrige a contagem, sem liberar memória.';
    get('regions').replaceChildren();
    for (const r of data.regions || []) row(get('regions'), [hex(r.start) + ' a ' + hex(r.end),
      r.kind === 1 ? 'SRAM principal' : r.kind === 2 ? 'RTC' : 'Não classificada', kib(r.end - r.start),
      kib(r.used), r.nested_used === undefined ? 'Não medido' : kib(r.nested_used), kib(r.free), kib(r.largest)]);
    get('allocation-config').replaceChildren();
    const configLabels = {heap_task_tracking: 'Rastreamento por tarefa (1 = ligado)',
      reserve_internal_bytes: 'Pool reservado para alocações internas/DMA', malloc_internal_threshold_bytes: 'Limiar para preferir malloc em RAM interna',
      wifi_static_rx_count: 'Wi-Fi: buffers estáticos de recepção', wifi_dynamic_rx_limit: 'Wi-Fi: limite de buffers dinâmicos de recepção (0 = sem limite)',
      wifi_static_tx_count: 'Wi-Fi: buffers estáticos de transmissão', wifi_dynamic_tx_limit: 'Wi-Fi: limite de buffers dinâmicos de transmissão',
      wifi_rx_ba_window: 'Wi-Fi: janela de recepção AMPDU', socket_limit: 'Limite de sockets',
      tls_in_limit_bytes: 'TLS: limite do conteúdo de entrada', tls_out_limit_bytes: 'TLS: limite do conteúdo de saída'};
    for (const [key, value] of Object.entries(data.allocation_config || {})) {
      row(get('allocation-config'), [configLabels[key] || key, key.endsWith('_bytes') ? kib(value) : String(value)]);
    }
    const owners = data.tasks.map(task => {
      const totals = {main: 0, nested: 0, dma: 0, rtc: 0, psram: 0, blocks: 0};
      for (const heap of task.heaps) {
        const kind = heapKind(heap, data.caps);
        if (kind === 'PSRAM') totals.psram += heap.current;
        if (kind === 'RTC') totals.rtc += heap.current;
        if (kind === 'SRAM principal') {
          totals.main += heap.current;
          for (const [address, size] of heap.blocks) totals.nested += nestedBytes(data, address, size);
          if (heap.caps & data.caps.dma) totals.dma += heap.current;
        }
        totals.blocks += heap.alloc_count;
      }
      const collected = task.heaps.reduce((sum, h) => sum + h.current, 0);
      return {...task, ...totals, totalsComplete: collected === task.current_requested};
    }).filter(task => task.current_requested > 0).sort((a, b) => b.main - a.main || b.psram - a.psram);
    get('owners').replaceChildren();
    for (const task of owners) {
      row(get('owners'), [task.name + ' ' + hex(task.handle), task.handle === 0 ? 'Inicialização antes do RTOS' : task.alive ? 'Ativa' : 'Encerrada com reservas',
        ...['main', 'nested', 'dma', 'rtc', 'psram'].map(key => task.totalsComplete ? kib(task[key]) : 'Coleta incompleta'), String(task.blocks)]);
    }
    const attributed = owners.reduce((sum, t) => sum + t.main, 0);
    get('owner-note').textContent = 'Solicitações atribuídas na SRAM principal: ' + kib(attributed) +
      '. Blocos ocupados medidos pelo alocador: ' + kib(layout.heap_allocated_blocks) +
      '. * Solicitações originais podem conter reservas de outros heaps, identificadas por endereço na coluna própria. Não compare sua soma diretamente à ocupação física corrigida. Diferenças também incluem arredondamento, metadados, rastreamento e mudanças entre as coletas; não são automaticamente vazamentos. ' +
      'Parte em heap DMA indica compatibilidade da região, não que a reserva tenha exigido DMA. ' +
      (data.complete && data.tracking_enabled ? 'Coleta dentro dos limites configurados.' : 'COLETA INCOMPLETA: rastreamento ausente ou limite de tarefas, heaps, blocos ou regiões atingido.');
    renderBlocks();
    renderFullComparison();
  }

  function renderBlocks() {
    if (!fullSnapshot) return;
    const blocks = allBlocks(fullSnapshot);
    blocks.sort((a, b) => b.size - a.size);
    const groups = new Map();
    for (const block of blocks) {
      const key = [block.owner, block.memory, block.size].join('|');
      if (!groups.has(key)) groups.set(key, {...block, count: 0, total: 0});
      const group = groups.get(key); group.count++; group.total += block.size;
    }
    get('block-groups').replaceChildren();
    for (const g of [...groups.values()].sort((a, b) => b.total - a.total).slice(0, 100)) {
      row(get('block-groups'), [g.owner, g.memory, g.size + ' bytes', String(g.count), kib(g.total)]);
    }
    get('block-list').replaceChildren();
    for (const block of blocks.slice(0, get('block-limit').value === 'all' ? blocks.length : 100)) {
      const nested = block.memory === 'SRAM principal' ? nestedBytes(fullSnapshot, block.address, block.size) : 0;
      row(get('block-list'), [block.owner, block.memory, hex(block.address), block.size + ' (' + kib(block.size) + ')',
        nested ? 'Contém heap interno: ' + kib(nested) : 'Não instrumentada']);
    }
  }

  function renderSymbols() {
    get('symbols').replaceChildren();
    if (!buildInventory?.verified) return;
    const symbols = buildInventory.symbols.filter(symbol => symbol.kind === 'sram');
    for (const symbol of symbols.slice(0, get('symbol-limit').value === 'all' ? symbols.length : 100)) {
      row(get('symbols'), [symbol.name, kib(symbol.size), symbol.section, symbol.owner]);
    }
  }

  async function fullFetch(kind, signal) {
    let url = kind === 'runtime' ? '/memory?full=1' : '/memory?build=1';
    if (desktop) {
      const ip = (typeof device !== 'undefined' && device?.ip) || document.getElementById('esp_ip')?.value || '';
      url = '/memory/detail?kind=' + kind + '&ip=' + encodeURIComponent(ip.trim());
    }
    const response = await fetch(url, {cache: 'no-store', signal});
    if (!response.ok) throw Error('Falha na coleta (' + response.status + '). Verifique o Wi-Fi e instale STEP03F_DIAG.');
    return response.json();
  }

  async function readFull() {
    if (reading || tasksReading || fullReading || !active || document.hidden) return;
    if (desktop && typeof busy !== 'undefined' && busy) {
      get('full-message').textContent = 'Aguarde o comando em andamento.';
      return;
    }
    fullReading = true;
    get('full-refresh').disabled = true;
    syncTimer();
    const requestGeneration = generation;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 60000);
    try {
      get('full-message').textContent = 'Coletando regiões de RAM, solicitantes, blocos e inventário do firmware...';
      const data = await fullFetch('runtime', controller.signal);
      if (requestGeneration !== generation || !active) return;
      const layoutKeys = ['physical_sram', 'icache', 'dcache', 'iram_image', 'dram_image', 'data', 'bss', 'noinit',
        'heap_extent', 'heap_allocated_blocks', 'heap_free_blocks', 'heap_admin', 'unclassified', 'rtc_heap_extent'];
      const uint = value => Number.isSafeInteger(value) && value >= 0 && value <= 0xffffffff;
      let heapCount = 0, blockCount = 0;
      if (data.v !== 1 || !data.layout || !data.caps || !Array.isArray(data.tasks) || data.tasks.length > 64 ||
          !Array.isArray(data.regions) || data.regions.length > 32 || data.regions.some(r =>
            !['start', 'end', 'kind', 'used', 'free', 'largest'].every(k => uint(r[k])) || r.end <= r.start ||
            (r.nested_used !== undefined && !uint(r.nested_used))) ||
          layoutKeys.some(key => !Number.isSafeInteger(data.layout[key])) || data.layout.physical_sram !== 524288 ||
          layoutKeys.filter(key => !['heap_admin', 'unclassified'].includes(key)).some(key => data.layout[key] < 0) ||
          ['internal', 'dma', 'rtc', 'psram'].some(key => !uint(data.caps[key]) || data.caps[key] === 0) ||
          !uint(data.diagnostic_psram_bytes) || !uint(data.scan_us) || typeof data.complete !== 'boolean' || typeof data.tracking_enabled !== 'boolean' ||
          data.tasks.some(t => typeof t.name !== 'string' || !uint(t.handle) || !uint(t.current_requested) ||
            !Array.isArray(t.heaps) || (heapCount += t.heaps.length) > 256 || t.heaps.some(h =>
              !uint(h.caps) || !uint(h.current) || !uint(h.alloc_count) || !Array.isArray(h.blocks) ||
              (blockCount += h.blocks.length) > 4096 || h.blocks.some(b => !Array.isArray(b) || b.length !== 2 || !uint(b[0]) || !uint(b[1]) || b[0] === 0 || b[1] === 0)))) {
        throw Error('Inventário inválido ou não disponível nesta versão.');
      }
      fullSnapshot = {...data, time: new Date().toISOString(), stage: get('full-stage').value || 'consulta'};
      if (fullHistory.length && (fullHistory[fullHistory.length - 1].uptime_ms > data.uptime_ms || fullHistory[fullHistory.length - 1].version !== data.version)) fullHistory.length = 0;
      fullHistory.push(fullSnapshot);
      if (fullHistory.length > 6) fullHistory.shift();
      buildInventory = null;
      get('components').replaceChildren(); get('symbols').replaceChildren(); get('objects').replaceChildren();
      renderFull(data);
      try {
        const build = await fullFetch('build', controller.signal);
        if (requestGeneration !== generation || !active) return;
        const verified = build.v === 1 && build.ready === true && build.version === data.version &&
          build.iram_reserved === data.layout.iram_image && build.dram_reserved === data.layout.dram_image &&
          uint(build.map_attributed_bytes) && uint(build.static_section_bytes) &&
          Array.isArray(build.components) && Array.isArray(build.symbols) &&
          !build.components.some(c => typeof c.name !== 'string' || !uint(c.size)) &&
          (build.objects === undefined || (Array.isArray(build.objects) && build.objects.every(o =>
            typeof o.name === 'string' && ['size', 'code', 'data', 'bss'].every(k => uint(o[k])) && o.code + o.data + o.bss === o.size))) &&
          !build.symbols.some(s => typeof s.name !== 'string' || !uint(s.size) || typeof s.section !== 'string' || typeof s.owner !== 'string');
        buildInventory = {...build, verified};
        if (verified) {
          const categoryCells = item => ['code', 'data', 'bss'].map(k => uint(item[k]) ? kib(item[k]) : 'Não disponível');
          for (const component of build.components) row(get('components'), [component.name, kib(component.size), ...categoryCells(component)]);
          for (const object of (build.objects || []).slice(0, 100)) row(get('objects'), [object.name, kib(object.size), ...categoryCells(object)]);
          renderSymbols();
          get('build-message').textContent = 'Inventário incorporado do ELF/map: ' + build.symbols.length + ' símbolos de RAM, ' +
            kib(build.map_attributed_bytes) + ' atribuídos a componentes, de ' + kib(build.static_section_bytes) +
            ' em seções da SRAM principal. Símbolos podem se sobrepor: não some seus tamanhos. Código e constantes que ficam na flash não entram nessa lista.';
        } else get('build-message').textContent = 'Inventário do ELF/map ausente ou incompatível. Use o instalador para gerar, incorporar e validar o relatório desta versão.';
      } catch (error) {
        get('build-message').textContent = 'Não foi possível obter o inventário do ELF/map: ' + error.message;
      }
      get('full-message').textContent = 'Inventário coletado às ' + new Date().toLocaleTimeString() +
        ' · espaço temporário de diagnóstico PSRAM ' + kib(data.diagnostic_psram_bytes) +
        ' · varredura e formatação ' + (data.scan_us / 1000).toFixed(1) +
        ' ms. Coleta sequencial: tarefas e alocações continuam mudando. Baixe o relatório para análise.';
    } catch (error) {
      if (requestGeneration === generation && active) get('full-message').textContent = 'Erro: ' + error.message + ' O inventário anterior pode estar desatualizado.';
    } finally {
      clearTimeout(timeout);
      fullReading = false;
      get('full-refresh').disabled = false;
      syncTimer();
    }
  }

  function select(showMemory) {
    generation++;
    active = showMemory;
    main.hidden = showMemory;
    panel.hidden = !showMemory;
    controls[0].setAttribute('aria-pressed', String(!showMemory));
    controls[1].setAttribute('aria-pressed', String(showMemory));
    syncTimer();
    if (showMemory) read();
  }
  controls[0].onclick = () => select(false);
  controls[1].onclick = () => select(true);
  get('refresh').onclick = () => read();
  get('save').onclick = () => read(get('stage').value);
  get('tasks-refresh').onclick = readTasks;
  get('full-refresh').onclick = readFull;
  get('block-limit').onchange = renderBlocks;
  get('symbol-limit').onchange = renderSymbols;
  get('auto').onchange = syncTimer;
  get('download').onclick = () => {
    if (!history.length && !taskSnapshot && !fullSnapshot && !buildInventory) {
      get('message').textContent = 'Faça uma consulta antes de baixar o relatório.';
      return;
    }
    const report = {format: 'jrbot-memory-v1', generated: new Date().toISOString(),
      units: 'bytes', note: 'DMA overlaps SRAM. RTC is separate from the main 512 KiB. Task attribution identifies the allocating task, not necessarily the consumer. Nested heaps are removed from physical totals only; task records remain original. Collection is sequential and tracing itself has overhead.', samples: history, task_snapshot: taskSnapshot, full_snapshot: fullSnapshot, full_snapshots: fullHistory, build_inventory: buildInventory};
    const url = URL.createObjectURL(new Blob([JSON.stringify(report, null, 2)], {type: 'application/json'}));
    const link = document.createElement('a');
    link.href = url;
    link.download = 'JrBot-memoria-' + Date.now() + '.json';
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1500);
  };
  document.addEventListener('visibilitychange', syncTimer);
  window.addEventListener('beforeunload', () => { active = false; generation++; syncTimer(); });
})();
