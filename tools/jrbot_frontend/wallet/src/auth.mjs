export function createAuthenticator(request, render, log = () => {}, clock = () => Date.now() / 1000) {
  let wallet = null, account = null, epoch = 0, pending = false, expires = 0;
  let message = '', invalidation = Promise.resolve();
  const emit = () => render({ authenticated: !!account && expires > clock(), pending, message,
    canSign: !!account && !!wallet?.features['solana:signMessage'], expires });
  function invalidate() {
    epoch++; expires = 0; pending = false; message = '';
    // Serialize revocations so a late logout cannot invalidate a newer authentication.
    invalidation = invalidation.then(() => request('logout', {})).catch(() => {});
  }
  return {
    observe(state) {
      const next = state.accounts.find(item => item.address === state.address) || null;
      if (wallet !== state.active || account?.address !== next?.address) {
        invalidate(); wallet = state.active; account = next;
      } else account = next;
      emit();
    },
    async authenticate() {
      if (!account || pending || !wallet?.features['solana:signMessage']) return;
      const attempt = epoch, selected = account, provider = wallet;
      pending = true; expires = 0; message = 'Preparando desafio...'; emit();
      try {
        await invalidation;
        if (attempt !== epoch) return;
        const challenge = await request('challenge', { address: selected.address });
        if (attempt !== epoch) return;
        const bytes = new TextEncoder().encode(challenge.message);
        message = 'Aprove a mensagem de autenticacao na extensao. Nao e uma compra.'; emit();
        const [output] = await provider.features['solana:signMessage'].signMessage({ account: selected, message: bytes });
        if (attempt !== epoch) return;
        if (!output || (output.signatureType && output.signatureType !== 'ed25519') ||
            output.signedMessage.length !== bytes.length || !bytes.every((byte, i) => output.signedMessage[i] === byte) ||
            output.signature.length !== 64) throw new Error('A carteira nao assinou exatamente o desafio esperado.');
        const signature = btoa(String.fromCharCode(...output.signature));
        const result = await request('verify', { id: challenge.id, signature });
        if (attempt !== epoch) return;
        if (result.authenticated !== true || result.address !== selected.address || !(result.expires_at > clock())) {
          throw new Error('O servidor nao confirmou a autenticacao desta carteira.');
        }
        expires = result.expires_at;
        message = 'Carteira autenticada nesta sessao local. Consulte a licenca ou o custo da compra na secao abaixo.';
        log('JR_WALLET_AUTH state=authenticated address=' + selected.address + ' expires_at=' + expires);
      } catch (error) {
        if (attempt !== epoch) return;
        expires = 0;
        message = error.code === 4001 ? 'Assinatura recusada. Carteira continua conectada, sem autenticacao.' :
          'Autenticacao nao confirmada: ' + (error.message || 'tente novamente');
        log('JR_WALLET_AUTH state=failed address=' + selected.address);
        invalidation = invalidation.then(() => request('logout', {})).catch(() => {});
      } finally {
        if (attempt === epoch) pending = false;
        emit();
      }
    },
    async check() {
      if (!account || !expires || pending) return;
      const attempt = epoch;
      if (expires <= clock()) {
        invalidate(); message = 'Sessao expirada. Clique Autenticar carteira novamente.'; emit(); return;
      }
      try {
        const result = await request('status');
        if (attempt !== epoch || pending) return;
        if (!result.authenticated || result.address !== account.address || result.expires_at <= clock()) {
          invalidate(); message = 'Sessao encerrada. Autentique novamente.';
          log('JR_WALLET_AUTH state=expired');
        }
      } catch (_) {
        if (attempt !== epoch || pending) return;
        invalidate(); message = 'Sem confirmacao do servidor. Autentique novamente.';
      }
      emit();
    }
  };
}
