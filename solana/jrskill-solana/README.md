# JrSkill Solana — Etapa 2

Prova isolada da Issue [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33), exclusivamente em `V1s-00`.

Arquitetura, limites, evidências e plano de teste: [docs/JRSKILL_SOLANA_STAGE2.md](../../docs/JRSKILL_SOLANA_STAGE2.md).

Este diretório armazena e recupera os bytes do JSON v1 já existente. Não executa a Skill nem resolve Recipes. A extensão comercial License PDA 00 está implementada, compilada, testada localmente e atualizada na Devnet; a primeira compra CLI com repasses e licença foi confirmada em 05/10/2026. Compra pelo painel e demais casos Devnet continuam pendentes. Scripts/crate identificam a entrega como 0.2.0; o upgrade foi publicado com metadados 0.1.0. Procedimento e limites: [checkpoint da compra 00](../../docs/JRSKILL_LICENSE_MODEL_00_TEST.md).

**Etapa 2 validada na Devnet em 2026-09-30:** publicação e leitura independente de 128 bytes, com `byte_equal: true`. Program ID: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`. PDA: `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`. As evidências completas estão no documento da Etapa 2.

## Preparação no VS Code / WSL

Requisitos no próprio WSL: Node.js 20+ com npm, Anchor CLI **1.1.2**, Rust e Solana CLI (ambiente observado: **3.1.10**). O Node instalado no Windows não substitui o Node no WSL. Se necessário, instale uma versão LTS pelo [guia oficial do Node.js](https://nodejs.org/en/download). Não rode `anchor init`: a estrutura já existe.

Na raiz do seu checkout (por exemplo, `/home/user/JrRobot`):

```bash
git switch V1s-00
git pull --ff-only origin V1s-00
cd solana/jrskill-solana
node --version
npm --version
anchor --version
solana --version
npm ci
npm test
```

## Program ID e build

O Program ID versionado agora é o programa Devnet validado. Para recompilar ou atualizar esse mesmo programa, preserve/restaure a chave **local** existente em `target/deploy/jrskill-keypair.json`; não gere uma nova no lugar dela. O Git contém apenas o endereço público. Uma nova chave representaria outro programa e outra implantação, fora deste checkpoint.

```bash
if [ "$(solana-keygen pubkey target/deploy/jrskill-keypair.json)" = "Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454" ]; then
  anchor keys sync --provider.cluster devnet
  anchor keys sync --provider.cluster localnet
  anchor build --provider.cluster devnet
  npm run test:idl
else
  echo "Chave ausente ou diferente: restaure a chave original antes de continuar."
fi
git diff -- Anchor.toml programs/jrskill/src/lib.rs
```

No Anchor 1.1.2, `anchor keys sync` atualiza o cluster selecionado; por isso execute explicitamente para **devnet e localnet**, usando a mesma chave. Isso alinha `declare_id!`, ambos os Program IDs em `Anchor.toml` e a chave local. O build/IDL usa o cluster selecionado, e um ID localnet diferente causa `DeclaredProgramIdMismatch` no teste mesmo quando devnet está correto. Confira esses IDs antes de deploy. `target/`, wallets, seed phrases e chaves privadas não entram no Git.

## Teste do programa no validator local

Antes da Devnet, execute o teste de criação/leitura e os casos negativos. Use uma wallet descartável **local**, diferente da wallet da Devnet, e um validator limpo (uma PDA existente faz o teste de primeira criação falhar):

```bash
if [ ! -f target/local-test-wallet.json ]; then
  solana-keygen new --no-bip39-passphrase --outfile target/local-test-wallet.json
fi
anchor test --validator legacy --provider.cluster localnet --provider.wallet target/local-test-wallet.json
```

O Anchor 1.1.2 usa Surfpool por padrão. `--validator legacy` seleciona o `solana-test-validator` fornecido pela Solana CLI, sem exigir instalação do Surfpool. O Anchor inicia esse validator, financia a wallet local e executa `npm run test:chain`. O teste recusa RPC que não seja localhost. Não execute `test:chain` na Devnet.

## Deploy e publicação na Devnet

A wallet pagadora deve existir em `~/.config/solana/id.json` e ter SOL de teste. Se ainda não existir, crie-a com `solana-keygen new` (preserve qualquer wallet existente). Para usar outra wallet, configure `ANCHOR_WALLET` e passe o mesmo caminho a `anchor deploy --provider.wallet`.

```bash
export ANCHOR_PROVIDER_URL=https://api.devnet.solana.com
export ANCHOR_WALLET="$HOME/.config/solana/id.json"
solana address --keypair "$ANCHOR_WALLET"
solana airdrop 2 --url devnet --keypair "$ANCHOR_WALLET"
solana balance --url devnet --keypair "$ANCHOR_WALLET"
anchor deploy --provider.cluster devnet --provider.wallet "$ANCHOR_WALLET"
npm run publish:devnet
npm run read:devnet -- "$(solana address --keypair "$ANCHOR_WALLET")"
```

O airdrop pode sofrer rate limit; saldo necessário depende do rent do programa e das taxas. Se faltar saldo, obtenha SOL **Devnet** pelo [faucet oficial](https://faucet.solana.com/) e confira o saldo antes de repetir o deploy.

Os scripts verificam o genesis hash da Devnet antes de operar. A publicação imprime tamanho da transação e rent da Skill; depois confirma a transação e verifica todos os campos. Uma segunda publicação só lê/confere a mesma PDA. Em caso de falha/timeout, rode primeiro a leitura para verificar se a transação já foi confirmada.

A leitura usa a public key do publisher, não precisa de chave privada e grava os bytes recuperados em `artifacts/<PDA>.json`, sem tocar no arquivo original. Ambas as operações retornam exit code diferente de zero se falharem.

## Evidência para enviar à Issue #33

Para demonstração pública, use o verificador independente:

```bash
node scripts/verify-public.mjs
```

Ele exige apenas Node.js 20+ e HTTPS para a RPC Devnet, sem wallet, chave privada, npm, IDL ou JSON local. Consulta a conta pública, valida o layout/hash do checkpoint e imprime o JSON recebido da blockchain. Comandos para qualquer diretório/servidor e saída esperada estão na seção [Public reproducible proof do Day 3](../../docs/HACKATHON_DEVLOG/day3.md#public-reproducible-proof--read-the-skill-without-a-wallet). O script é específico da prova congelada da Etapa 2.

Envie versões das ferramentas, commit testado, Program ID, public key da authority, PDA, assinatura, `payload_bytes`, `payload_hash`, `byte_equal`, resultados dos testes e logs de erro (se houver). Não envie conteúdo de keypair ou seed phrase.

O checkpoint de 2026-09-30 comprovou publicação e leitura independente na Devnet. Novas execuções devem manter a mesma comparação e evidências. A execução no JrBot pertence à Etapa 3.
