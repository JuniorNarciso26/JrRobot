# Conexao de carteira no painel JrBot

O painel usa `@wallet-standard/app` 1.1.1 para descobrir extensoes que anunciam Solana Devnet e as funcionalidades `standard:connect`, `standard:disconnect` e `standard:events`. Nao e uma lista fixa de marcas e nao garante compatibilidade com todas as versoes de todas as carteiras.

O bundle `../wallet_panel.js` e versionado e servido pelo Python local. O usuario do painel nao precisa instalar Node, pnpm ou pacotes; nao ha CDN em tempo de execucao.

## Desenvolvimento

Node 22+ e pnpm 11.25.0, neste diretorio:

```bash
pnpm install --frozen-lockfile --ignore-scripts
pnpm test
pnpm build
```

Depois de alterar os fontes, versionar tambem o bundle regenerado. O build usa esbuild 0.28.2. O lockfile fixa as dependencias.

`src/controller.mjs` gerencia somente conexao/contas. `src/panel.mjs` atualiza o DOM com `textContent`; nomes e enderecos da extensao nao sao interpolados como HTML. O controlador nao assina mensagens/transacoes, nao chama RPC e nao envia enderecos ao backend. O endereco publico aparece no log local baixado pelo usuario. Nao se persiste sessao de carteira nem se conecta automaticamente ao abrir o painel.

## Terceiros

O bundle inclui codigo de `@wallet-standard/app` 1.1.1, Solana Maintainers, distribuido sob Apache-2.0. A biblioteca nao foi modificada; esbuild somente a incorpora ao bundle. Licenca preservada em [WALLET_STANDARD_LICENSE.txt](WALLET_STANDARD_LICENSE.txt). Esta licenca refere-se a dependencia, nao altera a politica de licenciamento do projeto JrBot.

Fonte oficial: https://github.com/wallet-standard/wallet-standard

Prova manual e limites: [conexao de carteira, Etapa 4A](../../../docs/JRSKILL_WALLET_CONNECTION_TEST.md).
