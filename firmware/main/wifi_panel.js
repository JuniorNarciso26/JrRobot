(function () {
  'use strict';
  const main = document.querySelector('main');
  const nav = document.querySelector('.jr-memory-nav');
  const memory = document.getElementById('jr-memory-panel');
  if (!main || !nav || !memory || document.getElementById('jr-wifi-panel')) return;

  const style = document.createElement('style');
  style.textContent = '#jr-wifi-panel[hidden]{display:none!important}#jr-wifi-panel{max-width:1000px;margin:14px auto;padding:16px;color:inherit}#jr-wifi-panel h2{margin:0 0 10px}#jr-wifi-panel .wifi-tools{display:flex;gap:9px;flex-wrap:wrap;align-items:center;margin:10px 0}#jr-wifi-panel .wifi-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}#jr-wifi-panel .wifi-info{padding:10px;background:#111d32;border-radius:10px;min-width:0;overflow-wrap:anywhere}#jr-wifi-panel .wifi-info small{display:block;color:#a5b6ce;margin-bottom:4px}#jr-wifi-panel .wifi-list{display:grid;gap:8px;margin:12px 0}#jr-wifi-panel .wifi-item{border:1px solid #29415f;border-radius:10px;padding:10px;display:flex;justify-content:space-between;gap:10px;align-items:center;flex-wrap:wrap}#jr-wifi-panel label{display:block;margin-top:12px}#jr-wifi-panel input{width:100%;max-width:440px;background:#0e1a2e;color:#fff;border:1px solid #45617f;border-radius:8px;padding:10px}#jr-wifi-panel input[type=checkbox]{width:auto}#jr-wifi-panel .wifi-note{color:#9fb1ca;font-size:13px;margin:10px 0;line-height:1.5}#jr-wifi-panel .wifi-warning{border-left:3px solid #e4ab54;padding:8px 12px;margin:12px 0;background:#1b2231}#jr-wifi-panel .wifi-feedback{min-height:22px;margin:10px 0;color:#c3d6ef}#jr-wifi-panel .wifi-feedback.error{color:#ff9696}#jr-wifi-panel .wifi-actions{display:flex;gap:8px;flex-wrap:wrap}#jr-wifi-panel .wifi-delete{background:#4d2630;border:1px solid #a95869;color:#ffe2e7}#jr-wifi-panel button:disabled{opacity:.55;cursor:not-allowed}@media(max-width:560px){#jr-wifi-panel .wifi-grid{grid-template-columns:1fr}}';
  document.head.appendChild(style);

  const wifiTab = document.createElement('button');
  wifiTab.type = 'button';
  wifiTab.textContent = 'Wi-Fi';
  wifiTab.setAttribute('aria-pressed', 'false');
  nav.appendChild(wifiTab);

  const panel = document.createElement('section');
  panel.id = 'jr-wifi-panel';
  panel.className = 'card';
  panel.hidden = true;
  panel.innerHTML =
    '<h2>Wi-Fi do JrBot</h2><p class="wifi-note">Consulte a conexão atual, procure redes e troque a configuração depois da instalação inicial. Não é necessário usar o painel Serial.</p>' +
    '<div class="wifi-grid">' +
      '<div class="wifi-info"><small>Estado da conexão</small><strong id="jr-wifi-state">Aguardando</strong></div>' +
      '<div class="wifi-info"><small>Rede atual (SSID)</small><strong id="jr-wifi-ssid">—</strong></div>' +
      '<div class="wifi-info"><small>Endereço IP</small><strong id="jr-wifi-ip">—</strong></div>' +
      '<div class="wifi-info"><small>Sinal (RSSI)</small><strong id="jr-wifi-signal">—</strong></div>' +
      '<div class="wifi-info"><small>Canal</small><strong id="jr-wifi-channel">—</strong></div>' +
      '<div class="wifi-info"><small>Modo de operação</small><strong id="jr-wifi-mode">—</strong></div>' +
    '</div>' +
    '<div class="wifi-tools"><button type="button" id="jr-wifi-refresh">Atualizar status</button><label><input type="checkbox" id="jr-wifi-auto"> Atualizar sinal a cada 10 segundos</label></div>' +
    '<div id="jr-wifi-feedback" class="wifi-feedback" role="status">Selecione Wi-Fi para consultar o robô.</div>' +
    '<h3>Redes salvas</h3><p class="wifi-note">Até quatro redes. Selecione uma rede salva para usá-la na próxima inicialização. A rede atual continua ativa até o reinício.</p>' +
    '<div class="wifi-tools"><button type="button" id="jr-wifi-profiles-refresh">Atualizar redes salvas</button></div>' +
    '<div id="jr-wifi-profiles" class="wifi-list"></div>' +
    '<h3>Redes próximas</h3><p class="wifi-note">A busca é manual e fica indisponível durante a Live ou o modo autônomo, para não prejudicar o streaming. Lista limitada às oito primeiras redes encontradas.</p>' +
    '<div class="wifi-tools"><button type="button" id="jr-wifi-scan">Procurar redes</button></div>' +
    '<div id="jr-wifi-networks" class="wifi-list"></div>' +
    '<h3>Adicionar ou atualizar rede</h3><form id="jr-wifi-form">' +
      '<label>Nome da rede (SSID)<input id="jr-wifi-input-ssid" maxlength="32" autocomplete="off" required placeholder="Selecione acima ou digite o SSID"></label>' +
      '<label>Senha Wi-Fi<input id="jr-wifi-input-password" type="password" maxlength="64" autocomplete="new-password" placeholder="Senha WPA2 ou vazio para rede aberta"></label>' +
      '<p class="wifi-note">A senha não é exibida nas consultas e não fica salva no navegador. A rede será apenas armazenada; use o botão da lista de redes salvas para selecioná-la.</p>' +
      '<button type="submit" id="jr-wifi-save">Salvar rede sem desconectar</button>' +
    '</form>' +
    '<div class="wifi-warning"><strong>Aplicação da nova rede</strong><p id="jr-wifi-pending">Nenhuma troca pendente.</p><p>Ao reiniciar, esta página pode perder a conexão e o endereço IP pode mudar. Conecte o celular à nova rede e tente abrir <strong>https://jrbot.local</strong>. Se a rede não conectar, o JrBot possui recuperação pela configuração inicial.</p>' +
      '<button type="button" id="jr-wifi-restart" disabled>Confirmar e reiniciar para trocar</button></div>';
  main.before(panel);

  const el = name => document.getElementById('jr-wifi-' + name);
  const statusEl = name => el(name);
  const api = '/wifi-manager';
  let active = false, busy = false, timer = null;
  const signalText = rssi => {
    if (rssi >= -55) return 'Excelente';
    if (rssi >= -67) return 'Bom';
    if (rssi >= -75) return 'Regular';
    return 'Fraco';
  };
  const decodeSSID = encoded => {
    try { return decodeURIComponent(encoded || ''); }
    catch (_) { return '[SSID inválido]'; }
  };
  function feedback(message, error) {
    const node = el('feedback');
    node.textContent = message;
    node.className = 'wifi-feedback' + (error ? ' error' : '');
  }
  async function request(view, options) {
    const response = await fetch(api + (view ? '?view=' + encodeURIComponent(view) : ''), {
      cache: 'no-store',
      ...options
    });
    const body = await response.json();
    if (!response.ok || (body && body.ok === false)) throw Error(body.error || ('HTTP ' + response.status));
    return body;
  }
  async function write(json) {
    if (busy) return;
    if (!window.isSecureContext || location.protocol !== 'https:') {
      feedback('Abra a conexão por HTTPS para alterar redes.', true);
      return;
    }
    busy = true;
    let restarting = false;
    for (const button of panel.querySelectorAll('button')) button.disabled = true;
    try {
      const data = await request('', {method:'POST', headers:{'Content-Type':'application/json','X-JrBot-WiFi':'1'}, body:JSON.stringify(json)});
      if (data.restarting) {
        restarting = true;
        feedback('Reinício solicitado. Aguarde a nova conexão e abra https://jrbot.local na nova rede.');
      } else if (data.deleted) {
        feedback(json.selected
          ? 'Rede selecionada excluída. A conexão atual permanece ativa; no próximo reinício o JrBot poderá entrar na configuração inicial.'
          : 'Rede salva excluída. A conexão atual permanece ativa.');
      } else if (data.pending_restart) {
        feedback('Rede selecionada para o próximo reinício. A conexão atual permanece ativa.');
      } else {
        feedback('Rede armazenada. A conexão atual foi preservada.');
      }
      if (!restarting) await refresh();
    } catch (error) {
      feedback('Não foi possível alterar a rede: ' + error.message, true);
    } finally {
      busy = false;
      for (const button of panel.querySelectorAll('button')) button.disabled = false;
      // A restart may close HTTPS immediately; preserve the confirmation message.
      if (!restarting) {
        await status();
        await profiles();
      }
    }
  }
  async function status() {
    try {
      const data = await request('status');
      statusEl('state').textContent = data.connected ? 'Conectado' : 'Desconectado';
      statusEl('ssid').textContent = decodeSSID(data.ssid_pct);
      statusEl('ip').textContent = data.ip || '—';
      statusEl('signal').textContent = data.rssi_valid ? data.rssi + ' dBm · ' + signalText(data.rssi) : 'Sem leitura';
      statusEl('channel').textContent = data.channel || '—';
      statusEl('mode').textContent = data.mode || '—';
      el('pending').textContent = data.pending_restart
        ? 'Uma troca foi salva e depende de reinício. A conexão atual continua ativa.'
        : 'Nenhuma troca pendente.';
      el('restart').dataset.pending = data.pending_restart ? '1' : '';
      el('restart').disabled = !data.pending_restart || busy || data.mode !== 'idle';
      if (data.mode !== 'idle') el('pending').textContent += ' Pare a Live/autônomo para alterar Wi-Fi.';
      return data;
    } catch (error) {
      feedback('Não foi possível consultar o Wi-Fi: ' + error.message, true);
      return null;
    }
  }
  async function profiles() {
    try {
      const data = await request('profiles');
      const target = el('profiles');
      target.replaceChildren();
      if (!Array.isArray(data.profiles) || !data.profiles.length) {
        target.textContent = 'Nenhuma rede armazenada.';
        return;
      }
      for (const profile of data.profiles) {
        const item = document.createElement('div');
        item.className = 'wifi-item';
        const name = document.createElement('span');
        const ssid = decodeSSID(profile.ssid_pct);
        name.textContent = ssid + ' · ' + profile.mode + (profile.selected ? ' · Selecionada' : '');
        item.appendChild(name);
        const actions = document.createElement('div');
        actions.className = 'wifi-actions';
        const button = document.createElement('button');
        button.type = 'button';
        // Allow reapplying a selected SSID: its stored password may have changed.
        button.textContent = profile.selected ? 'Reaplicar configuração' : 'Usar esta rede';
        button.disabled = busy;
        button.onclick = () => {
          if (!confirm('Selecionar "' + ssid + '" para o próximo reinício? A conexão atual será preservada até reiniciar.')) return;
          write({action:'use', id:profile.id});
        };
        actions.appendChild(button);
        const remove = document.createElement('button');
        remove.type = 'button';
        remove.textContent = 'Excluir';
        remove.className = 'wifi-delete';
        remove.disabled = busy;
        remove.setAttribute('aria-label', 'Excluir rede ' + ssid);
        remove.onclick = () => {
          const warning = profile.selected
            ? 'ATENÇÃO: esta é a rede selecionada. Excluí-la não desconecta agora, mas no próximo reinício o JrBot poderá entrar na configuração inicial. Excluir "' + ssid + '"?'
            : 'Excluir a rede salva "' + ssid + '"? A conexão atual será preservada.';
          if (!confirm(warning)) return;
          write({action:'delete', id:profile.id, selected:!!profile.selected});
        };
        actions.appendChild(remove);
        item.appendChild(actions);
        target.appendChild(item);
      }
    } catch (error) {
      feedback('Erro consultando redes salvas: ' + error.message, true);
    }
  }
  async function scan() {
    if (busy) return;
    const button = el('scan');
    button.disabled = true;
    el('networks').textContent = 'Procurando redes…';
    try {
      const data = await request('scan');
      const target = el('networks');
      target.replaceChildren();
      if (!Array.isArray(data.networks) || !data.networks.length) {
        target.textContent = 'Nenhuma rede encontrada.';
        return;
      }
      for (const ap of data.networks) {
        const ssid = decodeSSID(ap.ssid_pct);
        const button = document.createElement('button');
        button.type = 'button';
        button.textContent = ssid + ' · ' + ap.rssi + ' dBm · canal ' + ap.channel;
        button.onclick = () => {
          el('input-ssid').value = ssid;
          el('input-password').value = '';
          el('input-password').focus();
          feedback('Rede selecionada: ' + ssid + '. Informe a senha e salve.');
        };
        target.appendChild(button);
      }
      if (data.limited) feedback('Busca limitada. Foram encontradas ' + data.total + ' redes; exibindo até oito.');
      else feedback('Busca de redes concluída.');
    } catch (error) {
      el('networks').textContent = '';
      feedback('Busca indisponível: ' + error.message, true);
    } finally {
      button.disabled = busy;
    }
  }
  async function refresh() {
    await status();
    await profiles();
  }
  function stopTimer() {
    if (timer !== null) { clearInterval(timer); timer = null; }
  }
  function syncTimer() {
    stopTimer();
    if (active && el('auto').checked && !document.hidden)
      timer = setInterval(() => { if (!busy) status(); }, 10000);
  }
  function selectWiFi() {
    nav.dispatchEvent(new Event('jrbot:wifi-selected'));
    for (const button of nav.querySelectorAll('button')) button.setAttribute('aria-pressed', String(button === wifiTab));
    main.hidden = true;
    memory.hidden = true;
    panel.hidden = false;
    active = true;
    refresh();
    syncTimer();
  }
  wifiTab.addEventListener('click', selectWiFi);
  for (const button of nav.querySelectorAll('button')) {
    if (button !== wifiTab)
      button.addEventListener('click', () => { active = false; panel.hidden = true; stopTimer(); });
  }
  el('refresh').onclick = status;
  el('profiles-refresh').onclick = profiles;
  el('scan').onclick = scan;
  el('auto').onchange = syncTimer;
  el('form').addEventListener('submit', async event => {
    event.preventDefault();
    const ssid = el('input-ssid').value.trim();
    const password = el('input-password').value;
    if (!ssid || ssid.length > 32 || password.length > 64 || (password.length > 0 && password.length < 8)) {
      feedback('Confira o nome e a senha (mínimo 8 caracteres para rede protegida).', true);
      return;
    }
    el('input-password').value = '';
    await write({action:'save', ssid, password, use_static:false});
  });
  el('restart').onclick = async () => {
    if (!el('restart').dataset.pending) return;
    if (!confirm('O JrBot irá reiniciar e poderá mudar de IP. Confirmar troca da rede Wi-Fi?')) return;
    await write({action:'restart'});
  };
  document.addEventListener('visibilitychange', syncTimer);
  window.addEventListener('beforeunload', stopTimer);
})();
