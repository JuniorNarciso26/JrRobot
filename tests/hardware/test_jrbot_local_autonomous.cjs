// Static checks for the UI served at http://jrbot.local/.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '..', '..');
const portal = fs.readFileSync(path.join(root, 'firmware/module_portal/jr_portal_web_v1.h'), 'utf8');
const parts = Array.from(portal.matchAll(/^"((?:\\.|[^"\\])*)";?$/gm), m => JSON.parse('"' + m[1] + '"'));
assert(parts.length > 10, 'Embedded HTML string missing');
const html = parts.join('');
for (const id of ['auto-on', 'auto-off', 'auto-state']) {
  assert(html.includes("id='" + id + "'"), 'Missing control ' + id);
}
for (const cmd of ['autonomo_on', 'autonomo_off', 'brain_status']) {
  assert(html.includes(cmd), 'Missing backend command ' + cmd);
}
assert(html.includes('setInterval(autoRefresh,5000)'), 'State polling missing');
assert(html.includes("q('auto-on').disabled=true"), 'Controls must disable while status is unknown');
const script = html.match(/<script>([\s\S]*?)<\/script>/);
assert(script, 'Embedded UI script missing');
new vm.Script(script[1], { filename: 'jrbot-local.js' });
console.log('OK JrBotLocal: on/off, current state, and JavaScript syntax');
