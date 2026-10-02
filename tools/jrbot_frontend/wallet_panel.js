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
      message = address ? "Conectada. Autenticacao, licencas e compra ainda nao implementadas." : "Sem conta Solana compativel com Devnet autorizada. Confira a extensao.";
      log("JR_WALLET state=" + (address ? "connected" : "disconnected") + " address=" + (address || "none") + " authenticated=false");
    }
    function refresh() {
      wallets2 = registry.get().filter(supportedWallet);
      if (active && !wallets2.includes(active)) {
        clear();
        message = "Carteira removida. Escolha outra extensao.";
        log("JR_WALLET state=removed authenticated=false");
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
          log("JR_WALLET state=connection_failed authenticated=false");
        } finally {
          if (attempt === generation) pending = false;
          emit();
        }
      },
      selectAccount(next) {
        if (pending || !accounts.some((account) => account.address === next)) return;
        address = next;
        log("JR_WALLET state=account_changed address=" + address + " authenticated=false");
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
          log("JR_WALLET state=disconnected authenticated=false");
          emit();
        }
      }
    };
  }

  // src/panel.mjs
  var element = (id) => document.getElementById(id);
  var walletSelect = element("wallet_provider");
  var accountSelect = element("wallet_account");
  var current;
  var copiedAddress = "";
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
  }, (line) => {
    if (typeof localLine === "function") localLine(line);
  });
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
  window.addEventListener("focus", () => controller.refresh());
})();
