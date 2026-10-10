const CHAIN = 'solana:devnet';
const SKILL = '8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux';
const PROGRAM = 'Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454';
const GENESIS = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';
const detail = error => String(error?.message || error || 'Erro sem detalhe').replace(/[\r\n\t]/g, ' ').replace(/[A-Za-z0-9+/=_-]{100,}/g, '[dados omitidos]').slice(0, 600);
export function sol(value) {
  const n = BigInt(value);
  return (n / 1000000000n).toString() + '.' + (n % 1000000000n).toString().padStart(9, '0');
}
export function createPurchase(request, render, log = () => {}, clock = () => Date.now() / 1000) {
  let wallet = null, account = null, authenticated = false, epoch = 0, pending = false;
  let quote = null, owned = false, accepted = false, signature = '', license = '', checked = false, sendConfirmed = false;
  let message = 'Conecte e autentique a carteira para consultar.';
  const canSign = () => !!wallet?.features['solana:signTransaction']?.supportedTransactionVersions?.includes('legacy') &&
    !!account?.features?.includes('solana:signTransaction') && !!account?.chains?.includes(CHAIN);
  const emit = () => render({ authenticated, pending, quote, owned, accepted, signature, license, checked, message,
    canQuote: authenticated && canSign() && !pending && !owned && !signature,
    canBuy: authenticated && canSign() && !pending && !owned && !signature && accepted && !!quote && quote.expires_at > clock() });
  function validate(result, address) {
    if (result.cluster !== 'devnet' || result.genesis !== GENESIS || result.program !== PROGRAM || result.skill !== SKILL ||
        result.buyer !== address || result.commitment !== 'finalized' || typeof result.owned !== 'boolean' || result.model_version !== 0) {
      throw new Error('Resposta de licenca/cotacao invalida');
    }
    for (const name of ['price_lamports', 'creator_lamports', 'treasury_lamports']) {
      if (!/^\d+$/.test(result[name])) throw new Error('Valores da oferta invalidos');
    }
    if (BigInt(result.price_lamports) !== BigInt(result.creator_lamports) + BigInt(result.treasury_lamports)) throw new Error('Repasses divergem do preco');
    return result;
  }
  const api = {
    observe(state, auth) {
      const next = state.accounts.find(item => item.address === state.address) || null;
      if (wallet !== state.active || account?.address !== next?.address || authenticated !== auth.authenticated) {
        epoch++; pending = false; quote = null; accepted = false; owned = false; signature = ''; license = ''; checked = false; sendConfirmed = false;
        message = 'Conecte e autentique a carteira para consultar.';
      }
      wallet = state.active; account = next; authenticated = !!auth.authenticated && !!next;
      if (authenticated && !canSign()) message = 'Esta carteira/conta nao oferece assinatura de transacao legacy Solana Devnet. A consulta de licenca continua disponivel.';
      emit();
    },
    accept(value) { accepted = !!value && !!quote; emit(); },
    async check() {
      if (!authenticated || pending) return;
      const attempt = epoch, address = account.address;
      pending = true; quote = null; accepted = false; message = 'Consultando licenca finalized na Devnet...'; emit();
      try {
        const result = validate(await request('status'), address);
        if (attempt !== epoch) return;
        owned = result.owned; license = result.license; checked = true;
        message = owned ? 'Licenca confirmada na Devnet para esta carteira.' : signature ?
          sendConfirmed ? 'RPC recebeu a transacao, licenca ainda nao confirmada. Consulte a transacao antes de tentar outro envio.' :
          'Envio nao confirmado; licenca ausente nesta consulta. Nao repita a compra; confira a carteira/Explorer.' : 'Esta carteira ainda nao possui a licenca.';
        log('JR_SKILL_LICENSE buyer=' + address + ' owned=' + owned + ' license=' + license);
      } catch (error) { if (attempt === epoch) { checked = false; owned = false; message = 'Consulta nao confirmada: ' + detail(error); log('JR_SKILL_PURCHASE stage=license_query state=error buyer=' + address + ' detail=' + detail(error)); } }
      finally { if (attempt === epoch) { pending = false; emit(); } }
    },
    async prepare() {
      if (!authenticated || !canSign() || pending || signature || owned) return;
      const attempt = epoch, address = account.address;
      pending = true; accepted = false; quote = null; message = 'Consultando termos e custo. Nenhuma compra sera enviada.'; emit();
      try {
        const result = validate(await request('quote', {}), address);
        if (attempt !== epoch) return;
        owned = result.owned; checked = true; license = result.license;
        if (!owned) {
          for (const key of ['rent_lamports', 'fee_lamports', 'total_lamports', 'priority_fee_limit_lamports', 'fee_limit_lamports', 'total_limit_lamports']) if (!/^\d+$/.test(result[key])) throw new Error('Custos invalidos');
          if (BigInt(result.total_lamports) !== BigInt(result.price_lamports) + BigInt(result.rent_lamports) + BigInt(result.fee_lamports) ||
              BigInt(result.priority_fee_limit_lamports) !== 100000n ||
              BigInt(result.fee_limit_lamports) !== BigInt(result.fee_lamports) + BigInt(result.priority_fee_limit_lamports) ||
              BigInt(result.total_limit_lamports) !== BigInt(result.price_lamports) + BigInt(result.rent_lamports) + BigInt(result.fee_limit_lamports) ||
              !(result.expires_at > clock()) || !result.quote_id || !result.transaction) throw new Error('Cotacao invalida/expirada');
          quote = result;
        }
        message = owned ? 'Licenca ja existente. Nenhuma compra necessaria.' : 'Confira os valores e marque o aceite antes de comprar. Cotacao valida por ate 90 segundos.';
      } catch (error) { if (attempt === epoch) { quote = null; message = 'Cotacao nao confirmada: ' + detail(error); log('JR_SKILL_PURCHASE stage=quote state=error buyer=' + address + ' detail=' + detail(error)); } }
      finally { if (attempt === epoch) { pending = false; emit(); } }
    },
    async buy() {
      if (!authenticated || !canSign() || pending || owned || signature || !accepted || !quote || quote.expires_at <= clock()) {
        if (quote && quote.expires_at <= clock()) { quote = null; accepted = false; message = 'Cotacao expirou. Consulte novamente.'; emit(); }
        return;
      }
      const attempt = epoch, selected = account, provider = wallet, terms = quote;
      let stage = 'wallet_signature';
      pending = true; accepted = false; message = 'Aprove a COMPRA na carteira. Ambiente: Solana Devnet.'; emit();
      log('JR_SKILL_PURCHASE stage=wallet_signature state=requested buyer=' + selected.address + ' chain=' + CHAIN);
      try {
        const transaction = Uint8Array.from(atob(terms.transaction), char => char.charCodeAt(0));
        const [output] = await provider.features['solana:signTransaction'].signTransaction({ account: selected, chain: CHAIN, transaction });
        if (attempt !== epoch) return;
        stage = 'signed_transaction_check';
        log('JR_SKILL_PURCHASE stage=wallet_signature state=returned buyer=' + selected.address);
        if (terms.expires_at <= clock()) throw new Error('Cotacao expirou durante a assinatura; nao enviada. Consulte novamente.');
        if (!(output?.signedTransaction instanceof Uint8Array) || output.signedTransaction.length > 1232) throw new Error('Transacao assinada invalida');
        message = 'Enviando transacao assinada para o RPC Devnet...'; emit();
        // An interrupted HTTP request may already have relayed: keep retry locked.
        signature = 'unknown';
        stage = 'submit_http';
        log('JR_SKILL_PURCHASE stage=submit_http state=requested buyer=' + selected.address);
        const result = await request('submit', { quote_id: terms.quote_id, signed_transaction: btoa(String.fromCharCode(...output.signedTransaction)) });
        if (attempt !== epoch) return;
        if (result.buyer !== selected.address || result.cluster !== 'devnet') throw new Error('Resposta de envio invalida');
        if (result.already_exists) { owned = true; checked = true; signature = ''; message = 'Licenca ja existente. Compra nao enviada.'; }
        else {
          if (!/^[1-9A-HJ-NP-Za-km-z]{80,90}$/.test(result.signature)) throw new Error('Assinatura de transacao invalida');
          signature = result.signature;
          if (result.state === 'submitted' && (!/^\d+$/.test(result.fee_lamports) || result.fee_limit_lamports !== terms.fee_limit_lamports ||
              BigInt(result.fee_lamports) > BigInt(terms.fee_limit_lamports))) throw new Error('Taxa devolvida pelo servidor diverge do teto aceito');
          sendConfirmed = result.state === 'submitted';
          message = result.state === 'submitted' ?
            'Transacao enviada; clique Consultar minha licenca para confirmar. Taxa verificada antes do envio: ' + sol(result.fee_lamports) + ' SOL. Em erro ou timeout, confira o Explorer antes de outro envio.' :
            'Envio nao confirmado pelo RPC. ' + detail(result.error_detail || 'Resposta RPC incerta') + '. Consulte a licenca e a transacao no Explorer antes de outro envio.';
          log('JR_SKILL_PURCHASE buyer=' + selected.address + ' signature=' + signature + ' state=' + result.state);
          log('JR_SKILL_PURCHASE stage=fee_check fee_lamports=' + result.fee_lamports + ' fee_limit_lamports=' + result.fee_limit_lamports);
          if (result.error_detail) log('JR_SKILL_PURCHASE stage=rpc_relay state=unknown signature=' + signature + ' detail=' + detail(result.error_detail));
        }
      } catch (error) {
        if (attempt !== epoch) return;
        log('JR_SKILL_PURCHASE stage=' + stage + ' state=error buyer=' + selected.address + ' signature=' + (signature || 'none') + ' detail=' + detail(error));
        message = signature ? 'Envio nao confirmado (' + stage + '): ' + detail(error) + '. Nao repita a compra; consulte a licenca e a carteira/Explorer.' :
          error.code === 4001 ? 'Compra recusada na carteira. Nenhuma transacao enviada pelo painel.' : 'Compra nao enviada (' + stage + '): ' + detail(error);
      } finally { if (attempt === epoch) { pending = false; quote = null; accepted = false; emit(); } }
    }
  };
  emit();
  return api;
}
