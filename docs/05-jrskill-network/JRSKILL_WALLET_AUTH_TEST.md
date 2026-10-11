# Stage 4B — Authenticate a Wallet by Message Signature

## Implementation — October 2, 2026

**Panel:** `JRBOT-PANEL-V1S-WALLET-05`, only on `V1s-00`. Wallet connection and authentication are separate steps. This work did **not** purchase a Skill, create a license, or authorize commercial Skill execution.

**Updated outcome:** User Log 73 confirmed real wallet authentication on October 2. The subsequent WALLET-06 release added Devnet/balance checks; see [Stage 4C](JRSKILL_WALLET_DEVNET_TEST.md). The controls and tests below describe the original WALLET-05 checkpoint.

## User-triggered flow

1. Connect a wallet and select **Authenticate Wallet**.
2. The local server generates a random nonce and binds a challenge to the session cookie, wallet address, and exact local origin.
3. The UTF-8 message includes the origin, address, Devnet environment, nonce, issue and expiry times, and the purpose: local authentication only, **without purchase or transfer**.
4. The extension asks the user to approve using Wallet Standard `solana:signMessage`.
5. The client requires an Ed25519 signature over a message identical to the server challenge. It submits the challenge ID and Base64 public signature to the local server.
6. The server verifies Ed25519 against the stored original message using the decoded Solana public address. The **backend**, not a browser-supplied flag, determines success.
7. Only a valid response for the same address changes the indicator to **Connected — Authenticated**.

Verification uses the Python `cryptography` library (test/pinned version **50.0.1**) instead of a custom cryptographic implementation. `PAINEL.bat` checks Ed25519 support and `pyserial` and installs requirements if needed. The server never imports private wallets or deployment keys.

## Session controls and limits

- Challenges expire after **120 seconds** and are single-use; a new challenge invalidates the previous one. Invalid signatures also consume the challenge.
- Authenticated sessions expire after **600 seconds**, without automatic renewal. New authentication requires a new signature.
- A random `HttpOnly; SameSite=Strict` cookie is scoped to `/jrskill/wallet`. The application was served through **loopback HTTP** (`localhost/127.0.0.1`); this is not a production public-hosting login system.
- POST requests require a local Host, an Origin matching that Host (including port), and the panel header. Challenge and session are origin-bound.
- Authentication state is in memory, with a **256-session** limit and expiry cleanup. Restarting the server terminates sessions.
- Account/extension changes, removal, or disconnection clear local authentication and request server revocation. Delayed signatures from a previously selected account are discarded.
- If the server is unavailable, remote revocation cannot be confirmed. The client drops its authenticated state, and the server-side record expires within ten minutes. A visible wallet address alone does not establish authentication.
- The client queries authenticated status every **10 seconds** and when the window regains focus. Failure, expiry, or revocation clears the indicator. Backend expiration applies independently of UI polling.
- Tabs of the same origin/profile share the cookie; switching or disconnecting in one tab can invalidate another. Authentication is not stored in localStorage.
- Logs may contain event names and public wallet addresses, **never** nonce, signature, or session token. The challenge identifier is discarded after use.

Local endpoints:

```text
POST /jrskill/wallet/challenge
POST /jrskill/wallet/verify
GET  /jrskill/wallet/status
POST /jrskill/wallet/logout
```

Licensing and purchasing remained future steps at this checkpoint. Local/Devnet experimental execution was independent of this session. Future licensed routes must validate both the server-side session **and the wallet's license**, rather than trusting a UI indicator.

## Original user test procedure

1. Close the previous panel, update `V1s-00`, and run `INSTALAR.bat panel`. Installing the new dependency may require Internet access.
2. Open `http://127.0.0.1:8765` in the same Chrome profile as the extension, confirm `JRBOT-PANEL-V1S-WALLET-05`, and use Ctrl+Shift+R if necessary.
3. Connect a disposable test wallet. The initial state must remain **Not authenticated**.
4. Click **Authenticate Wallet**, inspect the JrBot message for correct origin/address and the warning that no purchase or transfer is authorized, then approve in the extension.
5. Confirm **Connected — Authenticated**, download a text log, and record the result. No SOL or powered-on robot is required.
6. Disconnect and reconnect: the state must return to **Not authenticated**. Rejecting a signature should leave the wallet connected but unauthenticated.
7. If possible, change the extension account and verify reauthentication is required. Repeat after ten minutes to confirm expiry.

Never share private keys or seed phrases. An extension without `solana:signMessage` can connect but cannot authenticate under this scheme. Different message or signature prefixes are deliberately rejected.

## Validation performed and remaining evidence

- **6 Python authentication tests passed**, covering real Ed25519 signatures, expiry, different keys/messages, replay, origins/sessions, revocation/replacement, invalid addresses, session limits, and HTTP/cookie behavior using the panel's server path.
- **4 JavaScript authentication tests passed**, covering user-initiated approval, rejection/modified messages, account changes during signing, a response for another wallet, and server revocation.
- **17 previous connection, route, integrity, and executor tests passed**, for **27 automated tests** at this checkpoint.
- A headless Chrome test using a **simulated wallet** created an ephemeral WebCrypto key, signed a challenge, and received validation from the real Python backend. Connection, authentication, account-switch revocation, rejection/retry, and disconnection passed with no JS errors or Solana transaction. The ephemeral key was not saved.

**Real user result:** Log 73 records wallet connection and authentication confirmed by the server. It does **not** provide evidence for rejection, account-switching, or expiration on the real wallet extension. This checkpoint did not test robot hardware, purchase, firmware/program compilation, or Solana upgrade/deployment.

Official references: [Wallet Standard signMessage](https://github.com/solana-labs/wallet-standard/blob/master/packages/core/features/src/signMessage.ts), [cryptography Ed25519](https://cryptography.io/en/latest/hazmat/primitives/asymmetric/ed25519/).
