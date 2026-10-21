const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const source = fs.readFileSync('firmware/main/memory_panel.js', 'utf8');

class Node {
  constructor(doc) { this.doc = doc; this.children = []; this.hidden = false; this.checked = false; this.attrs = {}; this.textContent = ''; }
  set id(value) { this._id = value; this.doc.ids[value] = this; }
  get id() { return this._id; }
  setAttribute(key, value) { this.attrs[key] = value; }
  appendChild(child) { this.children.push(child); return child; }
  before(...nodes) { this.doc.inserted.push(...nodes); }
  replaceChildren(...nodes) { this.children = nodes; }
  remove() {}
  click() {}
  set innerHTML(value) {
    this.children = [];
    for (const match of value.matchAll(/<(\w+)([^>]*)>/g)) {
      const child = new Node(this.doc);
      child.tag = match[1];
      const id = /id="([^"]+)"/.exec(match[2]);
      if (id) child.id = id[1];
      if (child.id === 'jr-memory-stage') child.value = 'antes';
      this.children.push(child);
    }
  }
  querySelectorAll(tag) { return this.children.filter(child => child.tag === tag); }
}
function setup(desktop = false) {
  const doc = {ids: {}, inserted: [], hidden: false, listeners: {},
    getElementById(id) { return this.ids[id] || null; },
    createElement() { return new Node(this); },
    querySelector(selector) { return selector === 'main' ? this.main : null; },
    addEventListener(name, handler) { this.listeners[name] = handler; }};
  doc.main = new Node(doc); doc.head = new Node(doc); doc.body = new Node(doc);
  if (desktop) { doc.ids.esp_ip = new Node(doc); doc.ids.esp_ip.value = '192.168.0.123'; }
  const state = {requests: 0, timers: new Map(), blobs: [], reply: 'JR_MEMORY protocol=1 uptime_ms=12345'};
  state.fullReply = JSON.parse(fs.readFileSync('tests/memory/runtime_fixture.json', 'utf8'));
  state.buildReply = {v: 1, ready: true, version: state.fullReply.version,
    iram_reserved: state.fullReply.layout.iram_image, dram_reserved: state.fullReply.layout.dram_image,
    components: [{name: 'libwifi.a', size: 1024}], map_attributed_bytes: 1024, static_section_bytes: 1024,
    symbols: [{name: 'wifi_buffer', size: 1024, section: '.dram0.bss', kind: 'sram', owner: 'libwifi.a(wifi.o)'},
              {name: 'psram_buffer', size: 65536, section: '.ext_ram.bss', kind: 'psram_or_mapping', owner: 'libmain.a(main.o)'}]};
  state.taskReply = 'JR_TASKS ' + JSON.stringify({v: 1, total: 3, scan_us: 2500, workspace_bytes: 2400,
    rows: [[7, 1, 2, 1, 2048, 6144, 1], [9, 7, 1, 18, 8192, 25600, 1], [11, 0, 4, 1, 512, 0, 2]]});
  for (const key of ['internal', 'dma', 'psram']) {
    for (const [metric, value] of [['free', 8192], ['allocated', 16384], ['largest', 4096], ['min', 2048]]) state.reply += ` ${key}_${metric}=${value}`;
  }
  let timerId = 0;
  const context = {document: doc, window: {addEventListener() {}}, console, AbortController,
    Blob, URL: {createObjectURL(blob) {state.blobs.push(blob); return 'blob:test';}, revokeObjectURL() {}},
    setTimeout(fn, delay) { const id = ++timerId; state.timers.set(id, {fn, delay}); return id; },
    clearTimeout(id) { state.timers.delete(id); }, busy: false,
    async fetch(url) {
      assert.ok(['/memory', '/memory?tasks=1', '/memory?full=1', '/memory?build=1',
        '/memory/detail?kind=runtime&ip=192.168.0.123', '/memory/detail?kind=build&ip=192.168.0.123'].includes(url));
      state.requests++;
      return {ok: true, text: async () => url.includes('tasks=1') ? state.taskReply : state.reply,
        json: async () => url.includes('build') ? state.buildReply : state.fullReply};
    },
    async send(command) { assert.ok(['memoria', 'tarefas'].includes(command)); state.requests++; return command === 'tarefas' ? state.taskReply : state.reply; }};
  vm.runInNewContext(source, context);
  state.doc = doc; state.context = context;
  state.get = suffix => doc.getElementById('jr-memory-' + suffix);
  state.tabs = doc.inserted[0].querySelectorAll('button');
  return state;
}
const flush = () => new Promise(resolve => setImmediate(resolve));
(async () => {
  for (const desktop of [false, true]) {
    const s = setup(desktop);
    assert.equal(s.requests, 0, 'no request before opening memory');
    s.tabs[1].onclick(); await flush();
    assert.equal(s.requests, 1);
    assert.equal(s.doc.main.hidden, true);
    assert.equal(s.get('values').children[0].children[1].textContent, '8.0 KiB');
    s.get('stage').value = 'antes'; await s.get('save').onclick();
    s.get('stage').value = 'durante'; await s.get('save').onclick();
    assert.equal(s.get('history').children[2].children[1].textContent, 'durante');
    assert.match(s.get('comparison').textContent, /Durante em relação a Antes/);
    assert.match(s.get('accounting').textContent, /512 KiB/);
    await s.get('tasks-refresh').onclick();
    assert.equal(s.get('tasks').children.length, 3);
    assert.equal(s.get('tasks').children[0].children[0].textContent, 'jr_wa_peer #9');
    assert.equal(s.get('tasks').children[2].children[0].textContent, 'Tarefa #11');
    assert.match(s.get('tasks-message').textContent, /2.50 ms/);
    await s.get('full-refresh').onclick();
    assert.equal(s.get('layout').children.length, 9);
    assert.equal(s.get('owners').children.length, 2);
    assert.equal(s.get('owners').children[0].children[0].textContent, 'httpd"\\\n 0x3fc9c010');
    assert.equal(s.get('block-list').children.length, 2);
    assert.equal(s.get('components').children[0].children[0].textContent, 'libwifi.a');
    assert.equal(s.get('symbols').children.length, 1, 'PSRAM symbols excluded from main SRAM table');
    assert.match(s.get('layout-note').textContent, /fecham os 512 KiB/);
    s.get('auto').checked = true; s.get('auto').onchange();
    assert.ok([...s.timers.values()].some(t => t.delay === 5000));
    s.tabs[0].onclick();
    assert.equal(s.doc.main.hidden, false);
    assert.ok(![...s.timers.values()].some(t => t.delay === 5000));
    s.tabs[1].onclick(); await flush();
    for (let i = 0; i < 65; i++) await s.get('refresh').onclick();
    assert.equal(s.get('history').children.length, 60);
    s.get('download').onclick();
    const report = JSON.parse(await s.blobs[0].text());
    assert.equal(report.units, 'bytes'); assert.equal(report.samples.length, 60);
    assert.equal(report.task_snapshot.tasks[0].configured, 25600);
    assert.equal(report.full_snapshot.layout.unclassified, 0);
    assert.equal(report.build_inventory.symbols.length, 2);
    s.buildReply.version = 'stale';
    await s.get('full-refresh').onclick();
    assert.equal(s.get('components').children.length, 0, 'stale build attribution not displayed');
    assert.match(s.get('build-message').textContent, /incompatível/);
    s.taskReply = 'JR_TASKS ' + JSON.stringify({v: 1, total: 1, scan_us: 1, workspace_bytes: 1, rows: [[1, 999, 1, 1, 1, 1, 1]]});
    await s.get('tasks-refresh').onclick();
    assert.match(s.get('tasks-message').textContent, /inválida/);
    assert.equal(s.get('tasks').children.length, 3);
    s.reply = 'JR_ERROR comando_ou_argumentos_invalido';
    await s.get('refresh').onclick();
    assert.match(s.get('message').textContent, /STEP03C/);
    assert.equal(s.get('auto').checked, false);
    assert.equal(s.get('values').children.length, 3, 'last valid table retained');
    s.reply = 'JR_MEMORY protocol=1 uptime_ms=1';
    await s.get('refresh').onclick();
    assert.match(s.get('message').textContent, /incompleta/);
    s.doc.hidden = true;
    const requests = s.requests; await s.get('refresh').onclick();
    assert.equal(s.requests, requests, 'no read in hidden page');
  }
  console.log('PASS: HTTP and Serial views, readings, marked snapshots, bounded history, download, errors and timer pause');
})().catch(error => { console.error(error); process.exitCode = 1; });

