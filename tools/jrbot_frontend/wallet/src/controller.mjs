// Connection only: no signing, transactions, RPC, persistence or authentication.
const CHAIN = 'solana:devnet';
export function supportedWallet(wallet) {
  return wallet.chains?.includes(CHAIN) &&
    ['standard:connect', 'standard:disconnect', 'standard:events'].every(key => wallet.features?.[key]);
}
function accountsFor(wallet, accounts) {
  return (accounts || wallet.accounts || []).filter(account =>
    typeof account.address === 'string' && account.address.length > 0 && account.chains?.includes(CHAIN));
}
export function createWalletController(registry, changed, log = () => {}) {
  let wallets = [], active = null, accounts = [], address = '', pending = false;
  let message = 'Escolha uma carteira instalada e clique Conectar carteira.';
  let off = () => {}, generation = 0;
  function emit() { changed({ wallets, active, accounts, address, pending, message }); }
  function clear() {
    generation++;
    off(); off = () => {};
    active = null; accounts = []; address = ''; pending = false;
  }
  function setAccounts(next) {
    accounts = accountsFor(active, next);
    if (!accounts.some(account => account.address === address)) address = accounts[0]?.address || '';
    message = address ? 'Conectada. Autenticacao, licencas e compra ainda nao implementadas.' :
      'Sem conta Solana compativel com Devnet autorizada. Confira a extensao.';
    log('JR_WALLET state=' + (address ? 'connected' : 'disconnected') + ' address=' + (address || 'none') + ' authenticated=false');
  }
  function refresh() {
    wallets = registry.get().filter(supportedWallet);
    if (active && !wallets.includes(active)) {
      clear(); message = 'Carteira removida. Escolha outra extensao.';
      log('JR_WALLET state=removed authenticated=false');
    }
    if (!wallets.length) message = 'Nenhuma extensao Solana compativel detectada. Instale a Phantom no Chrome e recarregue o painel.';
    emit();
  }
  registry.on('register', refresh);
  registry.on('unregister', refresh);
  refresh();
  return {
    refresh,
    async connect(wallet) {
      if (pending || !wallets.includes(wallet)) return;
      const previous = active;
      clear(); pending = true; message = 'Aprove a conexao na extensao da carteira.';
      const attempt = generation;
      emit();
      try {
        if (previous) await previous.features['standard:disconnect'].disconnect();
        const result = await wallet.features['standard:connect'].connect();
        if (attempt !== generation) return;
        if (!registry.get().includes(wallet)) throw new Error('Wallet removed during connection');
        active = wallet;
        off = wallet.features['standard:events'].on('change', change => {
          if (active !== wallet) return;
          setAccounts(change.accounts ?? wallet.accounts);
          emit();
        });
        setAccounts(result.accounts);
      } catch (error) {
        if (attempt !== generation) return;
        clear();
        message = error.code === 4001 ? 'Conexao recusada. Nenhuma compra ou assinatura foi enviada.' :
          'Nao foi possivel conectar. Desbloqueie a extensao e tente novamente.';
        log('JR_WALLET state=connection_failed authenticated=false');
      } finally {
        if (attempt === generation) pending = false;
        emit();
      }
    },
    selectAccount(next) {
      if (pending || !accounts.some(account => account.address === next)) return;
      address = next;
      log('JR_WALLET state=account_changed address=' + address + ' authenticated=false');
      emit();
    },
    async disconnect() {
      if (pending) return;
      const previous = active;
      clear(); pending = true; message = 'Desconectando...'; emit();
      try { if (previous) await previous.features['standard:disconnect'].disconnect(); }
      catch (_) { message = 'Painel desconectado. Revogue tambem a conexao na extensao se necessario.'; }
      finally {
        pending = false;
        if (message === 'Desconectando...') message = 'Carteira desconectada do painel.';
        log('JR_WALLET state=disconnected authenticated=false'); emit();
      }
    }
  };
}
