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
    '<p>O RTOS identifica tarefas conhecidas pelo nome. Nomes repetidos, tarefas criadas durante a coleta e tarefas de bibliotecas sem identificação aparecem como Tarefa #ID. Não é uma lista de programas independentes nem de todo o consumo de RAM.</p>';
  main.setAttribute('data-jr-memory-main', '');
  main.before(nav, panel);
  const get = id => document.getElementById('jr-memory-' + id);
  const controls = nav.querySelectorAll('button');
  const history = [];
  let active = false, reading = false, timer = null, generation = 0;
  let tasksReading = false, taskSnapshot = null;
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
    for (const [prefix, label] of [['internal', 'RAM interna'], ['dma', 'DMA interna'], ['psram', 'PSRAM']]) {
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
    if (reading || tasksReading || !active || document.hidden) return;
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
    if (active && !document.hidden && get('auto').checked && !reading && !tasksReading) {
      timer = setTimeout(() => { timer = null; read(); }, 5000);
    }
  }

  async function read(stage) {
    if (reading || tasksReading || !active || document.hidden) return;
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
  get('auto').onchange = syncTimer;
  get('download').onclick = () => {
    if (!history.length && !taskSnapshot) {
      get('message').textContent = 'Faça uma consulta antes de baixar o relatório.';
      return;
    }
    const report = {format: 'jrbot-memory-v1', generated: new Date().toISOString(),
      units: 'bytes', note: 'DMA is a subset of internal RAM. Heap allocations exclude other physical RAM uses. Task configured stack budgets are source settings, not measured allocations.', samples: history, task_snapshot: taskSnapshot};
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
