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
    '<div class="memory-tools"><label>Marcar leitura <select id="jr-memory-stage"><option value="antes">Antes da live</option><option value="durante">Durante a live</option><option value="depois">Depois da live</option></select></label><button type="button" id="jr-memory-save">Salvar leitura</button><button type="button" id="jr-memory-download">Baixar relatório</button></div>' +
    '<p>As últimas 60 leituras ficam neste navegador até recarregar a página. Baixe o relatório para enviar à análise.</p>' +
    '<div class="memory-scroll"><table><thead><tr><th>Horário</th><th>Etapa</th><th>RAM livre</th><th>DMA livre</th><th>PSRAM livre</th></tr></thead><tbody id="jr-memory-history"></tbody></table></div>';
  main.setAttribute('data-jr-memory-main', '');
  main.before(nav, panel);
  const get = id => document.getElementById('jr-memory-' + id);
  const controls = nav.querySelectorAll('button');
  const history = [];
  let active = false, reading = false, timer = null, generation = 0;
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
  }

  function syncTimer() {
    if (timer !== null) clearTimeout(timer);
    timer = null;
    if (active && !document.hidden && get('auto').checked && !reading) {
      timer = setTimeout(() => { timer = null; read(); }, 5000);
    }
  }

  async function read(stage) {
    if (reading || !active || document.hidden) return;
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
  get('auto').onchange = syncTimer;
  get('download').onclick = () => {
    if (!history.length) {
      get('message').textContent = 'Faça uma consulta antes de baixar o relatório.';
      return;
    }
    const report = {format: 'jrbot-memory-v1', generated: new Date().toISOString(),
      units: 'bytes', note: 'DMA is a subset of internal RAM. Heap allocations exclude other physical RAM uses.', samples: history};
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
