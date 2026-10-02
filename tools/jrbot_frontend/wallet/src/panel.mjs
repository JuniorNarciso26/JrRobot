import { getWallets } from '@wallet-standard/app';
import { createWalletController } from './controller.mjs';
import { createAuthenticator } from './auth.mjs';
import { createNetworkCheck } from './network.mjs';

const element = id => document.getElementById(id);
const walletSelect = element('wallet_provider'), accountSelect = element('wallet_account');
let current, copiedAddress = '';
let authenticator;
let network, authState = { authenticated: false };
const localLog = line => { if (typeof localLine === 'function') localLine(line); };
function options(select, items, selected, placeholder) {
  select.replaceChildren();
  if (!items.length) {
    const option = document.createElement('option'); option.textContent = placeholder; option.value = '';
    select.appendChild(option);
  }
  for (const [value, label] of items) {
    const option = document.createElement('option'); option.value = value; option.textContent = label;
    select.appendChild(option);
  }
  select.value = selected;
}
const controller = createWalletController(getWallets(), state => {
  const previous = current?.wallets[Number(walletSelect.value)];
  current = state;
  const preferred = state.active || (state.wallets.includes(previous) ? previous : state.wallets[0]);
  options(walletSelect, state.wallets.map((wallet, i) => [String(i), wallet.name]),
    String(state.wallets.indexOf(preferred)), 'Nenhuma carteira detectada');
  options(accountSelect, state.accounts.map(account => [account.address, account.label || account.address]),
    state.address, 'Nenhuma conta autorizada');
  accountSelect.hidden = state.accounts.length < 2;
  element('wallet_account_label').hidden = accountSelect.hidden;
  element('wallet_address').textContent = state.address || 'Nenhuma carteira conectada';
  element('wallet_state').textContent = state.address ? 'Conectada — nao autenticada' : 'Desconectada';
  element('wallet_state').className = 'pill ' + (state.address ? 'ok' : '');
  element('wallet_msg').textContent = state.message;
  element('wallet_connect').textContent = state.active ? 'Trocar / reconectar carteira' : 'Conectar carteira';
  walletSelect.disabled = state.pending;
  accountSelect.disabled = state.pending;
  element('wallet_connect').disabled = state.pending || !state.wallets.length;
  element('wallet_disconnect').disabled = state.pending || !state.active;
  element('wallet_refresh').disabled = state.pending;
  element('wallet_copy').disabled = !state.address;
  if (copiedAddress !== state.address) element('wallet_copy').textContent = 'Copiar endereco';
  authenticator?.observe(state);
  network?.observe(state, authState);
}, localLog);

authenticator = createAuthenticator(async (operation, data) => {
  const options = data === undefined ? {} : { method: 'POST', headers: {
    'X-JrBot-Panel': '1', 'Content-Type': 'application/x-www-form-urlencoded'
  }, body: new URLSearchParams(data) };
  const response = await fetch('/jrskill/wallet/' + operation, { ...options, credentials: 'same-origin', signal: AbortSignal.timeout(10000) });
  if (!response.ok) throw new Error(await response.text());
  return response.json();
}, state => {
  authState = state;
  element('wallet_authenticate').disabled = !state.canSign || state.pending || current.pending;
  element('wallet_authenticate').textContent = state.pending ? 'Aguardando assinatura...' : state.authenticated ? 'Autenticar novamente' : 'Autenticar carteira';
  element('wallet_state').textContent = current.address ? (state.authenticated ? 'Conectada — autenticada' : 'Conectada — nao autenticada') : 'Desconectada';
  if (state.message) element('wallet_msg').textContent = state.message;
  else if (current.address && !state.canSign) element('wallet_msg').textContent = 'Esta carteira/conta nao oferece assinatura de mensagem Solana. Conexao mantida; autenticacao indisponivel.';
  network?.observe(current, state);
}, localLog);
authenticator.observe(current);
network = createNetworkCheck(async () => {
  const response = await fetch('/jrskill/wallet/devnet', { credentials: 'same-origin', signal: AbortSignal.timeout(15000) });
  if (!response.ok) throw new Error(await response.text());
  return response.json();
}, state => {
  element('wallet_network_check').disabled = !state.authenticated || !state.supported || state.pending;
  element('wallet_rpc').textContent = state.pending ? 'RPC: verificando...' : state.verified ? 'RPC: Solana Devnet confirmada' : 'RPC: nao confirmado';
  element('wallet_support').textContent = state.supported ? 'Carteira: suporte Devnet anunciado' : 'Carteira: suporte Devnet nao confirmado';
  element('wallet_balance').textContent = state.verified ? 'Saldo Devnet: ' + state.balance + ' SOL de teste' : 'Saldo Devnet: nao consultado';
  element('wallet_network_msg').textContent = state.message;
}, localLog);
network.observe(current, authState);
element('wallet_network_check').addEventListener('click', () => network.check());
element('wallet_authenticate').addEventListener('click', () => authenticator.authenticate());
setInterval(() => authenticator.check(), 10000);

element('wallet_connect').addEventListener('click', () => controller.connect(current.wallets[Number(walletSelect.value)]));
element('wallet_disconnect').addEventListener('click', () => controller.disconnect());
element('wallet_refresh').addEventListener('click', () => controller.refresh());
accountSelect.addEventListener('change', () => controller.selectAccount(accountSelect.value));
element('wallet_copy').addEventListener('click', async () => {
  const address = current.address;
  if (!address) return;
  try {
    await navigator.clipboard.writeText(address);
    if (current.address === address) { copiedAddress = address; element('wallet_copy').textContent = 'Endereco copiado'; }
  } catch (_) { element('wallet_msg').textContent = 'Selecione o endereco exibido e copie manualmente.'; }
});
window.addEventListener('focus', () => { controller.refresh(); authenticator.check(); });
