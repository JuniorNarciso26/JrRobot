(() => {
  // node_modules/.pnpm/@wallet-standard+app@1.1.1/node_modules/@wallet-standard/app/lib/esm/wallets.js
  var __classPrivateFieldGet = function(receiver, state, kind, f) {
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a getter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot read private member from an object whose class did not declare it");
    return kind === "m" ? f : kind === "a" ? f.call(receiver) : f ? f.value : state.get(receiver);
  };
  var __classPrivateFieldSet = function(receiver, state, value, kind, f) {
    if (kind === "m") throw new TypeError("Private method is not writable");
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a setter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot write private member to an object whose class did not declare it");
    return kind === "a" ? f.call(receiver, value) : f ? f.value = value : state.set(receiver, value), value;
  };
  var _AppReadyEvent_detail;
  var wallets = void 0;
  var registeredWalletsSet = /* @__PURE__ */ new Set();
  function addRegisteredWallet(wallet) {
    cachedWalletsArray = void 0;
    registeredWalletsSet.add(wallet);
  }
  function removeRegisteredWallet(wallet) {
    cachedWalletsArray = void 0;
    registeredWalletsSet.delete(wallet);
  }
  var listeners = {};
  function getWallets() {
    if (wallets)
      return wallets;
    wallets = Object.freeze({ register, get, on });
    if (typeof window === "undefined")
      return wallets;
    const api = Object.freeze({ register });
    try {
      window.addEventListener("wallet-standard:register-wallet", ({ detail: callback }) => callback(api));
    } catch (error) {
      console.error("wallet-standard:register-wallet event listener could not be added\n", error);
    }
    try {
      window.dispatchEvent(new AppReadyEvent(api));
    } catch (error) {
      console.error("wallet-standard:app-ready event could not be dispatched\n", error);
    }
    return wallets;
  }
  function register(...wallets2) {
    wallets2 = wallets2.filter((wallet) => !registeredWalletsSet.has(wallet));
    if (!wallets2.length)
      return () => {
      };
    wallets2.forEach((wallet) => addRegisteredWallet(wallet));
    listeners["register"]?.forEach((listener) => guard(() => listener(...wallets2)));
    return function unregister() {
      wallets2.forEach((wallet) => removeRegisteredWallet(wallet));
      listeners["unregister"]?.forEach((listener) => guard(() => listener(...wallets2)));
    };
  }
  var cachedWalletsArray;
  function get() {
    if (!cachedWalletsArray) {
      cachedWalletsArray = [...registeredWalletsSet];
    }
    return cachedWalletsArray;
  }
  function on(event, listener) {
    listeners[event]?.push(listener) || (listeners[event] = [listener]);
    return function off() {
      listeners[event] = listeners[event]?.filter((existingListener) => listener !== existingListener);
    };
  }
  function guard(callback) {
    try {
      callback();
    } catch (error) {
      console.error(error);
    }
  }
  var AppReadyEvent = class extends Event {
    get detail() {
      return __classPrivateFieldGet(this, _AppReadyEvent_detail, "f");
    }
    get type() {
      return "wallet-standard:app-ready";
    }
    constructor(api) {
      super("wallet-standard:app-ready", {
        bubbles: false,
        cancelable: false,
        composed: false
      });
      _AppReadyEvent_detail.set(this, void 0);
      __classPrivateFieldSet(this, _AppReadyEvent_detail, api, "f");
    }
    /** @deprecated */
    preventDefault() {
      throw new Error("preventDefault cannot be called");
    }
    /** @deprecated */
    stopImmediatePropagation() {
      throw new Error("stopImmediatePropagation cannot be called");
    }
    /** @deprecated */
    stopPropagation() {
      throw new Error("stopPropagation cannot be called");
    }
  };
  _AppReadyEvent_detail = /* @__PURE__ */ new WeakMap();

  // src/controller.mjs
  var CHAIN = "solana:devnet";
  function supportedWallet(wallet) {
    return wallet.chains?.includes(CHAIN) && ["standard:connect", "standard:disconnect", "standard:events"].every((key) => wallet.features?.[key]);
  }
  function accountsFor(wallet, accounts) {
    return (accounts || wallet.accounts || []).filter((account) => typeof account.address === "string" && account.address.length > 0 && account.chains?.includes(CHAIN));
  }
  function createWalletController(registry, changed, log = () => {
  }) {
    let wallets2 = [], active = null, accounts = [], address = "", pending = false;
    let message = "Escolha uma carteira instalada e clique Conectar carteira.";
    let off = () => {
    }, generation = 0;
    function emit() {
      changed({ wallets: wallets2, active, accounts, address, pending, message });
    }
    function clear() {
      generation++;
      off();
      off = () => {
      };
      active = null;
      accounts = [];
      address = "";
      pending = false;
    }
    function setAccounts(next) {
      accounts = accountsFor(active, next);
      if (!accounts.some((account) => account.address === address)) address = accounts[0]?.address || "";
      message = address ? "Conectada. Clique Autenticar carteira para confirmar o controle da conta." : "Sem conta Solana compativel com Devnet autorizada. Confira a extensao.";
      log("JR_WALLET state=" + (address ? "connected" : "disconnected") + " address=" + (address || "none") + " scope=connection");
    }
    function refresh() {
      wallets2 = registry.get().filter(supportedWallet);
      if (active && !wallets2.includes(active)) {
        clear();
        message = "Carteira removida. Escolha outra extensao.";
        log("JR_WALLET state=removed scope=connection");
      }
      if (!wallets2.length) message = "Nenhuma extensao Solana compativel detectada. Instale a Phantom no Chrome e recarregue o painel.";
      emit();
    }
    registry.on("register", refresh);
    registry.on("unregister", refresh);
    refresh();
    return {
      refresh,
      async connect(wallet) {
        if (pending || !wallets2.includes(wallet)) return;
        const previous = active;
        clear();
        pending = true;
        message = "Aprove a conexao na extensao da carteira.";
        const attempt = generation;
        emit();
        try {
          if (previous) await previous.features["standard:disconnect"].disconnect();
          const result = await wallet.features["standard:connect"].connect();
          if (attempt !== generation) return;
          if (!registry.get().includes(wallet)) throw new Error("Wallet removed during connection");
          active = wallet;
          off = wallet.features["standard:events"].on("change", (change) => {
            if (active !== wallet) return;
            setAccounts(change.accounts ?? wallet.accounts);
            emit();
          });
          setAccounts(result.accounts);
        } catch (error) {
          if (attempt !== generation) return;
          clear();
          message = error.code === 4001 ? "Conexao recusada. Nenhuma compra ou assinatura foi enviada." : "Nao foi possivel conectar. Desbloqueie a extensao e tente novamente.";
          log("JR_WALLET state=connection_failed scope=connection");
        } finally {
          if (attempt === generation) pending = false;
          emit();
        }
      },
      selectAccount(next) {
        if (pending || !accounts.some((account) => account.address === next)) return;
        address = next;
        log("JR_WALLET state=account_changed address=" + address + " scope=connection");
        emit();
      },
      async disconnect() {
        if (pending) return;
        const previous = active;
        clear();
        pending = true;
        message = "Desconectando...";
        emit();
        try {
          if (previous) await previous.features["standard:disconnect"].disconnect();
        } catch (_) {
          message = "Painel desconectado. Revogue tambem a conexao na extensao se necessario.";
        } finally {
          pending = false;
          if (message === "Desconectando...") message = "Carteira desconectada do painel.";
          log("JR_WALLET state=disconnected scope=connection");
          emit();
        }
      }
    };
  }

  // src/auth.mjs
  function createAuthenticator(request, render, log = () => {
  }, clock = () => Date.now() / 1e3) {
    let wallet = null, account = null, epoch = 0, pending = false, expires = 0;
    let message = "", invalidation = Promise.resolve();
    const emit = () => render({
      authenticated: !!account && expires > clock(),
      pending,
      message,
      canSign: !!account && !!wallet?.features["solana:signMessage"],
      expires
    });
    function invalidate() {
      epoch++;
      expires = 0;
      pending = false;
      message = "";
      invalidation = invalidation.then(() => request("logout", {})).catch(() => {
      });
    }
    return {
      observe(state) {
        const next = state.accounts.find((item) => item.address === state.address) || null;
        if (wallet !== state.active || account?.address !== next?.address) {
          invalidate();
          wallet = state.active;
          account = next;
        } else account = next;
        emit();
      },
      async authenticate() {
        if (!account || pending || !wallet?.features["solana:signMessage"]) return;
        const attempt = epoch, selected = account, provider = wallet;
        pending = true;
        expires = 0;
        message = "Preparando desafio...";
        emit();
        try {
          await invalidation;
          if (attempt !== epoch) return;
          const challenge = await request("challenge", { address: selected.address });
          if (attempt !== epoch) return;
          const bytes = new TextEncoder().encode(challenge.message);
          message = "Aprove a mensagem de autenticacao na extensao. Nao e uma compra.";
          emit();
          const [output] = await provider.features["solana:signMessage"].signMessage({ account: selected, message: bytes });
          if (attempt !== epoch) return;
          if (!output || output.signatureType && output.signatureType !== "ed25519" || output.signedMessage.length !== bytes.length || !bytes.every((byte, i) => output.signedMessage[i] === byte) || output.signature.length !== 64) throw new Error("A carteira nao assinou exatamente o desafio esperado.");
          const signature = btoa(String.fromCharCode(...output.signature));
          const result = await request("verify", { id: challenge.id, signature });
          if (attempt !== epoch) return;
          if (result.authenticated !== true || result.address !== selected.address || !(result.expires_at > clock())) {
            throw new Error("O servidor nao confirmou a autenticacao desta carteira.");
          }
          expires = result.expires_at;
          message = "Carteira autenticada nesta sessao local. Consulte a licenca ou o custo da compra na secao abaixo.";
          log("JR_WALLET_AUTH state=authenticated address=" + selected.address + " expires_at=" + expires);
        } catch (error) {
          if (attempt !== epoch) return;
          expires = 0;
          message = error.code === 4001 ? "Assinatura recusada. Carteira continua conectada, sem autenticacao." : "Autenticacao nao confirmada: " + (error.message || "tente novamente");
          log("JR_WALLET_AUTH state=failed address=" + selected.address);
          invalidation = invalidation.then(() => request("logout", {})).catch(() => {
          });
        } finally {
          if (attempt === epoch) pending = false;
          emit();
        }
      },
      async check() {
        if (!account || !expires || pending) return;
        const attempt = epoch;
        if (expires <= clock()) {
          invalidate();
          message = "Sessao expirada. Clique Autenticar carteira novamente.";
          emit();
          return;
        }
        try {
          const result = await request("status");
          if (attempt !== epoch || pending) return;
          if (!result.authenticated || result.address !== account.address || result.expires_at <= clock()) {
            invalidate();
            message = "Sessao encerrada. Autentique novamente.";
            log("JR_WALLET_AUTH state=expired");
          }
        } catch (_) {
          if (attempt !== epoch || pending) return;
          invalidate();
          message = "Sem confirmacao do servidor. Autentique novamente.";
        }
        emit();
      }
    };
  }

  // src/network.mjs
  var DEVNET = "EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG";
  function createNetworkCheck(request, render, log = () => {
  }) {
    let address = "", authenticated = false, supported = false, pending = false, epoch = 0;
    let message = "Conecte e autentique a carteira para consultar o saldo na Devnet.", verified = false, balance = "";
    function emit() {
      render({ authenticated, supported, pending, message, verified, balance });
    }
    const api = {
      observe(state, auth) {
        const next = state.address || "";
        const support = !!state.active?.chains.includes("solana:devnet") && !!state.accounts.find((item) => item.address === next)?.chains.includes("solana:devnet");
        if (next !== address || auth.authenticated !== authenticated || support !== supported) {
          epoch++;
          pending = false;
          verified = false;
          balance = "";
          message = "Conecte e autentique a carteira para consultar o saldo na Devnet.";
          address = next;
          authenticated = auth.authenticated;
          supported = support;
          emit();
          if (authenticated && supported) void api.check();
        } else emit();
      },
      async check() {
        if (!address || !authenticated || !supported || pending) return;
        const attempt = epoch, expected = address;
        pending = true;
        verified = false;
        balance = "";
        message = "Confirmando RPC Devnet e consultando saldo...";
        emit();
        try {
          const result = await request();
          if (attempt !== epoch) return;
          if (result.cluster !== "devnet" || result.genesis !== DEVNET || result.rpc_verified !== true || result.address !== expected || result.commitment !== "finalized" || !/^\d+$/.test(result.balance_lamports) || !/^\d+(\.\d{1,9})?$/.test(result.balance_sol)) {
            throw new Error("Resposta de rede/saldo invalida");
          }
          verified = true;
          balance = result.balance_sol;
          message = "Saldo consultado na Devnet. Isso nao confirma a rede selecionada na interface da extensao.";
          if (result.balance_lamports === "0") message += " Saldo zero: a carteira ainda nao possui SOL de teste nessa rede.";
          log("JR_WALLET_NETWORK rpc=devnet genesis_verified=true address=" + expected + " balance_lamports=" + result.balance_lamports + " rpc_slot=" + result.rpc_slot + " extension_network=unknown");
        } catch (error) {
          if (attempt !== epoch) return;
          verified = false;
          balance = "";
          message = "Devnet/saldo nao confirmados: " + error.message;
          log("JR_WALLET_NETWORK result=error");
        } finally {
          if (attempt === epoch) pending = false;
          emit();
        }
      }
    };
    emit();
    return api;
  }

  // src/purchase.mjs
  var CHAIN2 = "solana:devnet";
  var SKILL = "8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux";
  var PROGRAM = "Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454";
  var GENESIS = "EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG";
  var detail = (error) => String(error?.message || error || "Erro sem detalhe").replace(/[\r\n\t]/g, " ").replace(/[A-Za-z0-9+/=_-]{100,}/g, "[dados omitidos]").slice(0, 600);
  function sol(value) {
    const n = BigInt(value);
    return (n / 1000000000n).toString() + "." + (n % 1000000000n).toString().padStart(9, "0");
  }
  function createPurchase(request, render, log = () => {
  }, clock = () => Date.now() / 1e3) {
    let wallet = null, account = null, authenticated = false, epoch = 0, pending = false;
    let quote = null, owned = false, accepted = false, signature = "", license = "", checked = false, sendConfirmed = false;
    let message = "Conecte e autentique a carteira para consultar.";
    const canSign = () => !!wallet?.features["solana:signTransaction"]?.supportedTransactionVersions?.includes("legacy") && !!account?.features?.includes("solana:signTransaction") && !!account?.chains?.includes(CHAIN2);
    const emit = () => render({
      authenticated,
      pending,
      quote,
      owned,
      accepted,
      signature,
      license,
      checked,
      message,
      canQuote: authenticated && canSign() && !pending && !owned && !signature,
      canBuy: authenticated && canSign() && !pending && !owned && !signature && accepted && !!quote && quote.expires_at > clock()
    });
    function validate(result, address) {
      if (result.cluster !== "devnet" || result.genesis !== GENESIS || result.program !== PROGRAM || result.skill !== SKILL || result.buyer !== address || result.commitment !== "finalized" || typeof result.owned !== "boolean" || result.model_version !== 0) {
        throw new Error("Resposta de licenca/cotacao invalida");
      }
      for (const name of ["price_lamports", "creator_lamports", "treasury_lamports"]) {
        if (!/^\d+$/.test(result[name])) throw new Error("Valores da oferta invalidos");
      }
      if (BigInt(result.price_lamports) !== BigInt(result.creator_lamports) + BigInt(result.treasury_lamports)) throw new Error("Repasses divergem do preco");
      return result;
    }
    const api = {
      observe(state, auth) {
        const next = state.accounts.find((item) => item.address === state.address) || null;
        if (wallet !== state.active || account?.address !== next?.address || authenticated !== auth.authenticated) {
          epoch++;
          pending = false;
          quote = null;
          accepted = false;
          owned = false;
          signature = "";
          license = "";
          checked = false;
          sendConfirmed = false;
          message = "Conecte e autentique a carteira para consultar.";
        }
        wallet = state.active;
        account = next;
        authenticated = !!auth.authenticated && !!next;
        if (authenticated && !canSign()) message = "Esta carteira/conta nao oferece assinatura de transacao legacy Solana Devnet. A consulta de licenca continua disponivel.";
        emit();
      },
      accept(value) {
        accepted = !!value && !!quote;
        emit();
      },
      async check() {
        if (!authenticated || pending) return;
        const attempt = epoch, address = account.address;
        pending = true;
        quote = null;
        accepted = false;
        message = "Consultando licenca finalized na Devnet...";
        emit();
        try {
          const result = validate(await request("status"), address);
          if (attempt !== epoch) return;
          owned = result.owned;
          license = result.license;
          checked = true;
          message = owned ? "Licenca confirmada na Devnet para esta carteira." : signature ? sendConfirmed ? "RPC recebeu a transacao, licenca ainda nao confirmada. Consulte a transacao antes de tentar outro envio." : "Envio nao confirmado; licenca ausente nesta consulta. Nao repita a compra; confira a carteira/Explorer." : "Esta carteira ainda nao possui a licenca.";
          log("JR_SKILL_LICENSE buyer=" + address + " owned=" + owned + " license=" + license);
        } catch (error) {
          if (attempt === epoch) {
            checked = false;
            owned = false;
            message = "Consulta nao confirmada: " + detail(error);
            log("JR_SKILL_PURCHASE stage=license_query state=error buyer=" + address + " detail=" + detail(error));
          }
        } finally {
          if (attempt === epoch) {
            pending = false;
            emit();
          }
        }
      },
      async prepare() {
        if (!authenticated || !canSign() || pending || signature || owned) return;
        const attempt = epoch, address = account.address;
        pending = true;
        accepted = false;
        quote = null;
        message = "Consultando termos e custo. Nenhuma compra sera enviada.";
        emit();
        try {
          const result = validate(await request("quote", {}), address);
          if (attempt !== epoch) return;
          owned = result.owned;
          checked = true;
          license = result.license;
          if (!owned) {
            for (const key of ["rent_lamports", "fee_lamports", "total_lamports", "priority_fee_limit_lamports", "fee_limit_lamports", "total_limit_lamports"]) if (!/^\d+$/.test(result[key])) throw new Error("Custos invalidos");
            if (BigInt(result.total_lamports) !== BigInt(result.price_lamports) + BigInt(result.rent_lamports) + BigInt(result.fee_lamports) || BigInt(result.priority_fee_limit_lamports) !== 100000n || BigInt(result.fee_limit_lamports) !== BigInt(result.fee_lamports) + BigInt(result.priority_fee_limit_lamports) || BigInt(result.total_limit_lamports) !== BigInt(result.price_lamports) + BigInt(result.rent_lamports) + BigInt(result.fee_limit_lamports) || !(result.expires_at > clock()) || !result.quote_id || !result.transaction) throw new Error("Cotacao invalida/expirada");
            quote = result;
          }
          message = owned ? "Licenca ja existente. Nenhuma compra necessaria." : "Confira os valores e marque o aceite antes de comprar. Cotacao valida por ate 90 segundos.";
        } catch (error) {
          if (attempt === epoch) {
            quote = null;
            message = "Cotacao nao confirmada: " + detail(error);
            log("JR_SKILL_PURCHASE stage=quote state=error buyer=" + address + " detail=" + detail(error));
          }
        } finally {
          if (attempt === epoch) {
            pending = false;
            emit();
          }
        }
      },
      async buy() {
        if (!authenticated || !canSign() || pending || owned || signature || !accepted || !quote || quote.expires_at <= clock()) {
          if (quote && quote.expires_at <= clock()) {
            quote = null;
            accepted = false;
            message = "Cotacao expirou. Consulte novamente.";
            emit();
          }
          return;
        }
        const attempt = epoch, selected = account, provider = wallet, terms = quote;
        let stage = "wallet_signature";
        pending = true;
        accepted = false;
        message = "Aprove a COMPRA na carteira. Ambiente: Solana Devnet.";
        emit();
        log("JR_SKILL_PURCHASE stage=wallet_signature state=requested buyer=" + selected.address + " chain=" + CHAIN2);
        try {
          const transaction = Uint8Array.from(atob(terms.transaction), (char) => char.charCodeAt(0));
          const [output] = await provider.features["solana:signTransaction"].signTransaction({ account: selected, chain: CHAIN2, transaction });
          if (attempt !== epoch) return;
          stage = "signed_transaction_check";
          log("JR_SKILL_PURCHASE stage=wallet_signature state=returned buyer=" + selected.address);
          if (terms.expires_at <= clock()) throw new Error("Cotacao expirou durante a assinatura; nao enviada. Consulte novamente.");
          if (!(output?.signedTransaction instanceof Uint8Array) || output.signedTransaction.length > 1232) throw new Error("Transacao assinada invalida");
          message = "Enviando transacao assinada para o RPC Devnet...";
          emit();
          signature = "unknown";
          stage = "submit_http";
          log("JR_SKILL_PURCHASE stage=submit_http state=requested buyer=" + selected.address);
          const result = await request("submit", { quote_id: terms.quote_id, signed_transaction: btoa(String.fromCharCode(...output.signedTransaction)) });
          if (attempt !== epoch) return;
          if (result.buyer !== selected.address || result.cluster !== "devnet") throw new Error("Resposta de envio invalida");
          if (result.already_exists) {
            owned = true;
            checked = true;
            signature = "";
            message = "Licenca ja existente. Compra nao enviada.";
          } else {
            if (!/^[1-9A-HJ-NP-Za-km-z]{80,90}$/.test(result.signature)) throw new Error("Assinatura de transacao invalida");
            signature = result.signature;
            if (result.state === "submitted" && (!/^\d+$/.test(result.fee_lamports) || result.fee_limit_lamports !== terms.fee_limit_lamports || BigInt(result.fee_lamports) > BigInt(terms.fee_limit_lamports))) throw new Error("Taxa devolvida pelo servidor diverge do teto aceito");
            sendConfirmed = result.state === "submitted";
            message = result.state === "submitted" ? "Transacao enviada; clique Consultar minha licenca para confirmar. Taxa verificada antes do envio: " + sol(result.fee_lamports) + " SOL. Em erro ou timeout, confira o Explorer antes de outro envio." : "Envio nao confirmado pelo RPC. " + detail(result.error_detail || "Resposta RPC incerta") + ". Consulte a licenca e a transacao no Explorer antes de outro envio.";
            log("JR_SKILL_PURCHASE buyer=" + selected.address + " signature=" + signature + " state=" + result.state);
            log("JR_SKILL_PURCHASE stage=fee_check fee_lamports=" + result.fee_lamports + " fee_limit_lamports=" + result.fee_limit_lamports);
            if (result.error_detail) log("JR_SKILL_PURCHASE stage=rpc_relay state=unknown signature=" + signature + " detail=" + detail(result.error_detail));
          }
        } catch (error) {
          if (attempt !== epoch) return;
          log("JR_SKILL_PURCHASE stage=" + stage + " state=error buyer=" + selected.address + " signature=" + (signature || "none") + " detail=" + detail(error));
          message = signature ? "Envio nao confirmado (" + stage + "): " + detail(error) + ". Nao repita a compra; consulte a licenca e a carteira/Explorer." : error.code === 4001 ? "Compra recusada na carteira. Nenhuma transacao enviada pelo painel." : "Compra nao enviada (" + stage + "): " + detail(error);
        } finally {
          if (attempt === epoch) {
            pending = false;
            quote = null;
            accepted = false;
            emit();
          }
        }
      }
    };
    emit();
    return api;
  }

  // src/panel.mjs
  var element = (id) => document.getElementById(id);
  var walletSelect = element("wallet_provider");
  var accountSelect = element("wallet_account");
  var current;
  var copiedAddress = "";
  var authenticator;
  var network;
  var authState = { authenticated: false };
  var purchase;
  var localLog = (line) => {
    if (typeof localLine === "function") localLine(line);
  };
  function options(select, items, selected, placeholder) {
    select.replaceChildren();
    if (!items.length) {
      const option = document.createElement("option");
      option.textContent = placeholder;
      option.value = "";
      select.appendChild(option);
    }
    for (const [value, label] of items) {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = label;
      select.appendChild(option);
    }
    select.value = selected;
  }
  var controller = createWalletController(getWallets(), (state) => {
    const previous = current?.wallets[Number(walletSelect.value)];
    current = state;
    const preferred = state.active || (state.wallets.includes(previous) ? previous : state.wallets[0]);
    options(
      walletSelect,
      state.wallets.map((wallet, i) => [String(i), wallet.name]),
      String(state.wallets.indexOf(preferred)),
      "Nenhuma carteira detectada"
    );
    options(
      accountSelect,
      state.accounts.map((account) => [account.address, account.label || account.address]),
      state.address,
      "Nenhuma conta autorizada"
    );
    accountSelect.hidden = state.accounts.length < 2;
    element("wallet_account_label").hidden = accountSelect.hidden;
    element("wallet_address").textContent = state.address || "Nenhuma carteira conectada";
    element("wallet_state").textContent = state.address ? "Conectada \u2014 nao autenticada" : "Desconectada";
    element("wallet_state").className = "pill " + (state.address ? "ok" : "");
    element("wallet_msg").textContent = state.message;
    element("wallet_connect").textContent = state.active ? "Trocar / reconectar carteira" : "Conectar carteira";
    walletSelect.disabled = state.pending;
    accountSelect.disabled = state.pending;
    element("wallet_connect").disabled = state.pending || !state.wallets.length;
    element("wallet_disconnect").disabled = state.pending || !state.active;
    element("wallet_refresh").disabled = state.pending;
    element("wallet_copy").disabled = !state.address;
    if (copiedAddress !== state.address) element("wallet_copy").textContent = "Copiar endereco";
    authenticator?.observe(state);
    network?.observe(state, authState);
    purchase?.observe(state, authState);
  }, localLog);
  authenticator = createAuthenticator(async (operation, data) => {
    const options2 = data === void 0 ? {} : { method: "POST", headers: {
      "X-JrBot-Panel": "1",
      "Content-Type": "application/x-www-form-urlencoded"
    }, body: new URLSearchParams(data) };
    const response = await fetch("/jrskill/wallet/" + operation, { ...options2, credentials: "same-origin", signal: AbortSignal.timeout(1e4) });
    if (!response.ok) throw new Error(await response.text());
    return response.json();
  }, (state) => {
    authState = state;
    element("wallet_authenticate").disabled = !state.canSign || state.pending || current.pending;
    element("wallet_authenticate").textContent = state.pending ? "Aguardando assinatura..." : state.authenticated ? "Autenticar novamente" : "Autenticar carteira";
    element("wallet_state").textContent = current.address ? state.authenticated ? "Conectada \u2014 autenticada" : "Conectada \u2014 nao autenticada" : "Desconectada";
    if (state.message) element("wallet_msg").textContent = state.message;
    else if (current.address && !state.canSign) element("wallet_msg").textContent = "Esta carteira/conta nao oferece assinatura de mensagem Solana. Conexao mantida; autenticacao indisponivel.";
    network?.observe(current, state);
    purchase?.observe(current, state);
  }, localLog);
  authenticator.observe(current);
  network = createNetworkCheck(async () => {
    const response = await fetch("/jrskill/wallet/devnet", { credentials: "same-origin", signal: AbortSignal.timeout(15e3) });
    if (!response.ok) throw new Error(await response.text());
    return response.json();
  }, (state) => {
    element("wallet_network_check").disabled = !state.authenticated || !state.supported || state.pending;
    element("wallet_rpc").textContent = state.pending ? "RPC: verificando..." : state.verified ? "RPC: Solana Devnet confirmada" : "RPC: nao confirmado";
    element("wallet_support").textContent = state.supported ? "Carteira: suporte Devnet anunciado" : "Carteira: suporte Devnet nao confirmado";
    element("wallet_balance").textContent = state.verified ? "Saldo Devnet: " + state.balance + " SOL de teste" : "Saldo Devnet: nao consultado";
    element("wallet_network_msg").textContent = state.message;
  }, localLog);
  network.observe(current, authState);
  purchase = createPurchase(async (operation, data) => {
    const options2 = data === void 0 ? {} : { method: "POST", headers: {
      "X-JrBot-Panel": "1",
      "Content-Type": "application/x-www-form-urlencoded"
    }, body: new URLSearchParams(data) };
    const response = await fetch(
      "/jrskill/wallet/purchase/" + operation,
      { ...options2, credentials: "same-origin", signal: AbortSignal.timeout(6e4) }
    );
    if (!response.ok) throw new Error("HTTP " + response.status + ": " + await response.text());
    return response.json();
  }, (state) => {
    element("purchase_check").disabled = !state.authenticated || state.pending;
    element("purchase_quote").disabled = !state.canQuote;
    element("purchase_buy").disabled = !state.canBuy;
    element("purchase_accept").disabled = !state.quote || state.pending;
    element("purchase_accept").checked = state.accepted;
    element("purchase_msg").textContent = state.message;
    element("purchase_license").textContent = !state.checked ? "Licenca nao consultada." : state.owned ? "minimal_recipe_01 \u2014 Licenciada nesta carteira. PDA: " + state.license : "Esta carteira ainda nao possui a Skill de teste.";
    element("purchase_terms").textContent = state.quote ? "Comprador: " + state.quote.buyer + "\nPreco: " + sol(state.quote.price_lamports) + " SOL de teste\nCriador: " + state.quote.creator + " \u2014 " + sol(state.quote.creator_lamports) + " SOL\nJrBot: " + state.quote.treasury + " \u2014 " + sol(state.quote.treasury_lamports) + " SOL\nDeposito da licenca: " + sol(state.quote.rent_lamports) + " SOL\nTaxa de rede estimada sem prioridade: " + sol(state.quote.fee_lamports) + " SOL\nPrioridade adicional permitida: ate " + sol(state.quote.priority_fee_limit_lamports) + " SOL\nTeto aceito da taxa de rede: " + sol(state.quote.fee_limit_lamports) + " SOL\nTotal estimado sem prioridade: " + sol(state.quote.total_lamports) + " SOL de teste\nTotal maximo autorizado: " + sol(state.quote.total_limit_lamports) + " SOL de teste" : "";
    const link = element("purchase_transaction");
    link.hidden = !state.signature || state.signature === "unknown";
    if (!link.hidden) link.href = "https://explorer.solana.com/tx/" + state.signature + "?cluster=devnet";
  }, localLog);
  purchase.observe(current, authState);
  element("purchase_check").addEventListener("click", () => purchase.check());
  element("purchase_quote").addEventListener("click", () => purchase.prepare());
  element("purchase_accept").addEventListener("change", (event) => purchase.accept(event.target.checked));
  element("purchase_buy").addEventListener("click", () => purchase.buy());
  element("wallet_network_check").addEventListener("click", () => network.check());
  element("wallet_authenticate").addEventListener("click", () => authenticator.authenticate());
  setInterval(() => authenticator.check(), 1e4);
  element("wallet_connect").addEventListener("click", () => controller.connect(current.wallets[Number(walletSelect.value)]));
  element("wallet_disconnect").addEventListener("click", () => controller.disconnect());
  element("wallet_refresh").addEventListener("click", () => controller.refresh());
  accountSelect.addEventListener("change", () => controller.selectAccount(accountSelect.value));
  element("wallet_copy").addEventListener("click", async () => {
    const address = current.address;
    if (!address) return;
    try {
      await navigator.clipboard.writeText(address);
      if (current.address === address) {
        copiedAddress = address;
        element("wallet_copy").textContent = "Endereco copiado";
      }
    } catch (_) {
      element("wallet_msg").textContent = "Selecione o endereco exibido e copie manualmente.";
    }
  });
  window.addEventListener("focus", () => {
    controller.refresh();
    authenticator.check();
  });
})();
