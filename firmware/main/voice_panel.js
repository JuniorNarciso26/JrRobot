/* JrBot V2 - Voice control (lazy on-demand vTTS, no external cloud). */
(function () {
  'use strict';
  const main = document.querySelector('main');
  const nav = document.querySelector('.jr-memory-nav');
  const memory = document.getElementById('jr-memory-panel');
  const wifi = document.getElementById('jr-wifi-panel');
  if (!main || !nav || !memory || document.getElementById('jr-voice-panel')) return;

  const css = document.createElement('style');
  css.textContent = '#jr-voice-panel[hidden]{display:none!important}#jr-voice-panel{max-width:1000px;margin:14px auto;padding:16px}#jr-voice-panel .voice-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}#jr-voice-panel label{display:block;font-size:13px;color:#b4c9e2}#jr-voice-panel label span{float:right;font-weight:700;color:#f1f7ff}#jr-voice-panel input[type=range]{width:100%;margin:12px 0 4px}#jr-voice-panel select,#jr-voice-panel textarea{width:100%;margin-top:6px;background:#0e1a2e;border:1px solid #45617f;border-radius:9px;color:#fff;padding:10px}#jr-voice-panel textarea{min-height:82px;resize:vertical;font:inherit}#jr-voice-panel .voice-actions{display:flex;gap:10px;flex-wrap:wrap;margin:12px 0}#jr-voice-panel .voice-help{color:#a6bad2;font-size:13px;line-height:1.5}#jr-voice-panel #jr-voice-feedback{min-height:24px;margin:12px 0;color:#d4e4fb}#jr-voice-panel #jr-voice-feedback.error{color:#ff9292}#jr-voice-panel .voice-indicators{display:flex;gap:10px;flex-wrap:wrap;margin-top:10px}#jr-voice-panel .voice-indicators span{background:#12223a;border:1px solid #334966;border-radius:10px;padding:8px}#jr-voice-panel button:disabled,#jr-voice-panel input:disabled,#jr-voice-panel select:disabled{opacity:.5;cursor:not-allowed}@media(max-width:580px){#jr-voice-panel .voice-grid{grid-template-columns:1fr}}';
  document.head.appendChild(css);

  const tab = document.createElement('button');
  tab.type = 'button';
  tab.textContent = 'Voz';
  tab.setAttribute('aria-pressed', 'false');
  nav.appendChild(tab);
  const panel = document.createElement('section');
  panel.id = 'jr-voice-panel';
  panel.className = 'card';
  panel.hidden = true;
  panel.innerHTML =
    '<h2>Voz do JrBot · vTTS pt-BR offline</h2>' +
    '<p class="voice-help">Personalize a voz sintetizada pelo ESP32-S3, sem internet. Os ajustes ficam salvos; a fala manual inicia desligada após reiniciar. No modo Autônomo, o comando Olá ESP recebe uma resposta de voz curta.</p>' +
    '<label><input type="checkbox" id="jr-voice-enabled"> Ativar sintetizador de voz</label>' +
    '<div class="voice-grid" style="margin-top:15px">' +
      '<label>Modelo<select id="jr-voice-model"><option value="s">Lite (S) · menor consumo</option><option value="m">Quality (M) · melhor qualidade</option></select></label>' +
      '<div></div>' +
      '<label>Volume <span id="jr-voice-vol-value">75%</span><input id="jr-voice-vol" type="range" min="0" max="100" step="1" value="75"></label>' +
      '<label>Ganho digital <span id="jr-voice-gain-value">6 dB</span><input id="jr-voice-gain" type="range" min="0" max="24" step="1" value="6"></label>' +
      '<label>Velocidade <span id="jr-voice-speed-value">1.0×</span><input id="jr-voice-speed" type="range" min="0.7" max="1.6" step="0.1" value="1.0"></label>' +
      '<label>Expressividade <span id="jr-voice-expr-value">1.0×</span><input id="jr-voice-expr" type="range" min="0.5" max="2.5" step="0.1" value="1.0"></label>' +
      '<label>Suavizar agudos <span id="jr-voice-debuzz-value">0 dB</span><input id="jr-voice-debuzz" type="range" min="0" max="12" step="1" value="0"></label>' +
    '</div>' +
    '<div class="voice-actions"><button type="button" id="jr-voice-save" class="primary">Salvar ajustes</button><button type="button" id="jr-voice-refresh">Consultar estado</button></div>' +
    '<label>Texto de teste (até 1.200 bytes)<textarea id="jr-voice-text" maxlength="1200">Olá! Eu sou o JrBot e agora tenho minha própria voz.</textarea></label>' +
    '<div class="voice-actions"><button type="button" id="jr-voice-speak" class="primary">Fazer o JrBot falar</button></div>' +
    '<p class="voice-help">Para usar o botão Fazer o JrBot falar, desative Autônomo/WakeNet e encerre a Live. No modo Autônomo, Olá ESP já recebe uma resposta falada e a escuta retorna após a reprodução. A resposta automática usa a voz Lite S (pré-gerada na PSRAM ao ativar Autônomo); o volume, ganho e filtro continuam ajustáveis. Ganho alto pode distorcer; comece em 6 dB.</p>' +
    '<div id="jr-voice-feedback" role="status">Selecione a aba para consultar o sintetizador.</div>' +
    '<div class="voice-indicators"><span id="jr-voice-engine">Motor: aguardando</span><span id="jr-voice-busy">Estado: aguardando</span><span id="jr-voice-latency">Início da fala: —</span><span id="jr-voice-rtf">RTF: —</span><span id="jr-voice-psram">PSRAM livre: —</span></div>';
  main.before(panel);
  const get = name => document.getElementById('jr-voice-' + name);
  const settingsKeys = ['enabled', 'model', 'vol', 'gain', 'speed', 'expr', 'debuzz'];
  const readSettings = () => ({
    enabled: get('enabled').checked,
    model: get('model').value,
    volume: +get('vol').value,
    gain_db: +get('gain').value,
    debuzz_db: +get('debuzz').value,
    speed: +get('speed').value,
    expression: +get('expr').value
  });
  let active = false, busy = false, available = false, speaking = false, timer = null;
  function feedback(message, error) {
    get('feedback').textContent = message;
    get('feedback').className = error ? 'error' : '';
  }
  function values() {
    get('vol-value').textContent = get('vol').value + '%';
    get('gain-value').textContent = get('gain').value + ' dB';
    get('speed-value').textContent = Number(get('speed').value).toFixed(1) + '×';
    get('expr-value').textContent = Number(get('expr').value).toFixed(1) + '×';
    get('debuzz-value').textContent = get('debuzz').value + ' dB';
  }
  function controls() {
    for (const key of settingsKeys) get(key).disabled = busy || !available;
    get('save').disabled = busy || !available;
    get('speak').disabled = busy || !available || !get('enabled').checked || speaking;
    get('refresh').disabled = busy;
  }
  async function request(payload) {
    if (payload && location.protocol !== 'https:') {
      throw Error('Abra https://jrbot.local para alterar a voz.');
    }
    const response = await fetch('/voice', payload ? {
      method: 'POST', cache: 'no-store',
      headers: {'Content-Type': 'application/json', 'X-JrBot-Voice': '1'},
      body: JSON.stringify(payload)
    } : {cache: 'no-store'});
    const result = await response.json();
    if (!response.ok || result.ok === false) throw Error(result.error === 'ESP_ERR_INVALID_SIZE' ? 'O áudio excedeu a memória disponível. Reduza o texto; nenhuma fala parcial foi reproduzida.' : result.error || 'HTTP ' + response.status);
    return result;
  }
  function apply(data, overwrite) {
    available = !!data.available;
    speaking = !!data.busy;
    if (overwrite) {
      get('enabled').checked = !!data.enabled;
      get('model').value = data.model === 'm' ? 'm' : 's';
      get('vol').value = data.volume;
      get('gain').value = data.gain_db;
      get('debuzz').value = data.debuzz_db;
      get('speed').value = data.speed;
      get('expr').value = data.expression;
      values();
    }
    get('engine').textContent = 'Motor: ' + (available ? 'vTTS disponível' : 'não incluído nesta compilação');
    get('busy').textContent = 'Estado: ' + (speaking ? 'falando…' : data.enabled ? 'ligado' : 'desligado');
    get('rtf').textContent = 'RTF: ' + (data.utterances ? Number(data.last_rtf).toFixed(2) : '—');
    get('latency').textContent = 'Início da fala: ' + (data.last_first_audio_ms ? data.last_first_audio_ms + ' ms' : '—');
    get('psram').textContent = 'PSRAM livre: ' + (data.psram_free / 1024 / 1024).toFixed(1) + ' MiB';
    if (!available) feedback('O painel está disponível, mas o firmware ainda não tem os modelos vTTS. Prepare o build de integração antes de ativar.', true);
    else if (speaking) feedback('O JrBot está sintetizando ou reproduzindo a frase.');
    else if (data.last_error && data.last_error !== 'ESP_OK' && data.utterances >= 0) feedback(data.last_error === 'ESP_ERR_INVALID_SIZE' ? 'O áudio excedeu a memória disponível. Reduza o texto; nenhuma fala parcial foi reproduzida.' : 'Último resultado da fala: ' + data.last_error, true);
    controls();
  }
  async function refresh(overwrite) {
    if (!active || busy) return;
    try {
      const data = await request();
      apply(data, overwrite);
      if (data.busy || speaking) startTimer();
    } catch (error) { feedback('Falha ao consultar o robô: ' + error.message, true); }
  }
  async function write(payload, message) {
    if (busy) return;
    busy = true;
    controls();
    try {
      const result = await request(payload);
      if (result.queued) {
        speaking = true;
        feedback('Frase recebida. O JrBot está preparando o áudio completo antes de falar.');
      } else feedback(message);
      await refreshOnce(result.queued ? false : true);
    } catch (error) { feedback('Não foi possível aplicar: ' + error.message, true); }
    finally { busy = false; controls(); }
  }
  async function refreshOnce(overwrite) {
    const data = await request();
    apply(data, overwrite);
  }
  function startTimer() {
    if (timer) clearInterval(timer);
    timer = setInterval(async () => {
      if (!active || busy) return;
      try {
        const data = await request();
        const wasSpeaking = speaking;
        apply(data, false);
        if (wasSpeaking && !data.busy && data.last_error === 'ESP_OK')
          feedback('Fala concluída. RTF ' + Number(data.last_rtf).toFixed(2) + ', preparo ' + data.last_first_audio_ms + ' ms, áudio ' + data.last_audio_ms + ' ms.');
      } catch (error) { feedback('Falha ao consultar o estado: ' + error.message, true); }
      if (!speaking && timer) { clearInterval(timer); timer = null; }
    }, 1200);
  }
  function selectVoice() {
    nav.dispatchEvent(new Event('jrbot:wifi-selected')); /* pause memory reading */
    nav.dispatchEvent(new Event('jrbot:voice-selected')); /* pause Wi-Fi polling */
    for (const button of nav.querySelectorAll('button'))
      button.setAttribute('aria-pressed', String(button === tab));
    main.hidden = true;
    memory.hidden = true;
    if (wifi) wifi.hidden = true;
    panel.hidden = false;
    active = true;
    refresh(true);
  }
  tab.onclick = selectVoice;
  for (const button of nav.querySelectorAll('button')) {
    if (button === tab) continue;
    button.addEventListener('click', () => {
      active = false;
      panel.hidden = true;
      if (timer) { clearInterval(timer); timer = null; }
    });
  }
  for (const key of ['vol', 'gain', 'speed', 'expr', 'debuzz']) get(key).addEventListener('input', values);
  get('enabled').onchange = controls;
  get('save').onclick = async () => write({action:'configure', ...readSettings()}, 'Configuração de voz atualizada.');
  get('speak').onclick = async () => {
    const phrase = get('text').value.trim();
    if (!phrase || new TextEncoder().encode(phrase).length > 1200) {
      feedback('Digite um texto de até 1.200 bytes UTF-8.', true);
      return;
    }
    await write({action:'speak', text:phrase}, 'Frase enviada.');
    if (speaking) startTimer();
  };
  get('refresh').onclick = () => refresh(true);
  document.addEventListener('visibilitychange', () => { if (document.hidden && timer) { clearInterval(timer); timer = null; } else if (active && speaking) startTimer(); });
  values();
  controls();
})();
