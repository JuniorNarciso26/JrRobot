# JrBot Panel — Wallet Connection

The panel uses `@wallet-standard/app` **1.1.1** to discover wallet extensions that advertise Solana Devnet and the `standard:connect`, `standard:disconnect`, and `standard:events` features. This is not a hard-coded list of wallet brands and does not guarantee compatibility with every version of every wallet.

The `../wallet_panel.js` bundle is versioned and served by the local Python application. Panel users do **not** need to install Node, pnpm, or packages. No runtime CDN is required.

## Development

Use Node 22+ and pnpm 11.25.0 in this directory:

```bash
pnpm install --frozen-lockfile --ignore-scripts
pnpm test
pnpm build
```

After modifying sources, commit the regenerated bundle as well. The build uses esbuild 0.28.2, and dependencies are pinned by the lockfile.

`src/controller.mjs` manages wallet connections and accounts only. `src/auth.mjs` requests a message signature after a user click and verifies it through the local server. `src/panel.mjs` updates the DOM with `textContent`, without interpolating extension-provided names or addresses into HTML.

The public wallet address and signature are sent only to the local authentication server. That flow does **not** sign transactions or use RPC. The public address appears in logs downloaded by the user, but challenge, signature, and cookie values are not logged. Opening the panel does not automatically connect or authenticate.

## Third-party license

The bundle includes `@wallet-standard/app` 1.1.1 by Solana Maintainers, licensed under Apache-2.0. The library is unmodified; esbuild only includes it in the output bundle. The original license is preserved in [WALLET_STANDARD_LICENSE.txt](WALLET_STANDARD_LICENSE.txt). That dependency license does not change the JrBot project's own license.

Official source: https://github.com/wallet-standard/wallet-standard

Manual validation records and limitations: [Wallet Connection — Stage 4A](../../../docs/05-jrskill-network/JRSKILL_WALLET_CONNECTION_TEST.md) and [Wallet Authentication — Stage 4B](../../../docs/05-jrskill-network/JRSKILL_WALLET_AUTH_TEST.md).

## Purchase — Stage 4D

`src/purchase.mjs` manages quotations, explicit acceptance, `solana:signTransaction` signing with chain `solana:devnet`, and license lookup for the test Skill. The `wallet_purchase.py` backend constructs the unsigned transaction, verifies the signature/message/session, and submits to the Devnet RPC.

Authentication remains separate from purchase. No private key is supplied to the panel. See the [Purchase Architecture and Manual Test](../../../docs/05-jrskill-network/JRSKILL_WALLET_PURCHASE_TEST.md).
