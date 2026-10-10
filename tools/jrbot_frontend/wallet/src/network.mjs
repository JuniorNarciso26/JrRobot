const DEVNET = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';
export function createNetworkCheck(request, render, log = () => {}) {
  let address = '', authenticated = false, supported = false, pending = false, epoch = 0;
  let message = 'Conecte e autentique a carteira para consultar o saldo na Devnet.', verified = false, balance = '';
  function emit() { render({ authenticated, supported, pending, message, verified, balance }); }
  const api = {
    observe(state, auth) {
      const next = state.address || '';
      const support = !!state.active?.chains.includes('solana:devnet') &&
        !!state.accounts.find(item => item.address === next)?.chains.includes('solana:devnet');
      if (next !== address || auth.authenticated !== authenticated || support !== supported) {
        epoch++; pending = false; verified = false; balance = '';
        message = 'Conecte e autentique a carteira para consultar o saldo na Devnet.';
        address = next; authenticated = auth.authenticated; supported = support;
        emit();
        if (authenticated && supported) void api.check();
      } else emit();
    },
    async check() {
      if (!address || !authenticated || !supported || pending) return;
      const attempt = epoch, expected = address;
      pending = true; verified = false; balance = ''; message = 'Confirmando RPC Devnet e consultando saldo...'; emit();
      try {
        const result = await request();
        if (attempt !== epoch) return;
        if (result.cluster !== 'devnet' || result.genesis !== DEVNET || result.rpc_verified !== true ||
            result.address !== expected || result.commitment !== 'finalized' ||
            !/^\d+$/.test(result.balance_lamports) || !/^\d+(\.\d{1,9})?$/.test(result.balance_sol)) {
          throw new Error('Resposta de rede/saldo invalida');
        }
        verified = true; balance = result.balance_sol;
        message = 'Saldo consultado na Devnet. Isso nao confirma a rede selecionada na interface da extensao.';
        if (result.balance_lamports === '0') message += ' Saldo zero: a carteira ainda nao possui SOL de teste nessa rede.';
        log('JR_WALLET_NETWORK rpc=devnet genesis_verified=true address=' + expected +
          ' balance_lamports=' + result.balance_lamports + ' rpc_slot=' + result.rpc_slot + ' extension_network=unknown');
      } catch (error) {
        if (attempt !== epoch) return;
        verified = false; balance = ''; message = 'Devnet/saldo nao confirmados: ' + error.message;
        log('JR_WALLET_NETWORK result=error');
      } finally { if (attempt === epoch) pending = false; emit(); }
    }
  };
  emit();
  return api;
}
