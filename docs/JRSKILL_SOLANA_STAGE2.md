# JrSkill Network — Etapa 2: Skill PDA na Solana Devnet

Data: **2026-09-30**. Linha: **V1s-00**. Registro técnico: [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Objetivo e baseline

Publicar o arquivo `tools/jrbot_frontend/jrskill/skills/minimal_recipe_01.json` e recuperá-lo byte a byte, sem alterar o JSON v1 validado na Etapa 1.

Estado remoto conferido antes da implementação: `V1s-00` em `6fee26ec0e1168207ce245c41731db0ca7ac9859`. A Issue #33 estava aberta, com a prova local validada e a Skill PDA ainda como próxima etapa. As referências antigas a `test/jrbot-v1s-00` na Issue são históricas; este checkpoint usa somente `V1s-00`.

Blob original no Git (LF): **128 bytes**, SHA-256 **`416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`**. Cópia Windows usada nos testes locais (CRLF por configuração de checkout): **137 bytes**, SHA-256 **`b4b75ec8bf66012e7d3308d046d230998eb99cdc2b2386d4299791f5ed3f0714`**. O blob versionado não foi alterado. Os scripts calculam o hash dos bytes lidos do arquivo, inclusive espaços e quebras de linha. Não usam `JSON.stringify`, minificação ou expansão da Recipe. Uma diferença de terminação de linha entre checkouts gera outro hash/PDA; a prova compara exatamente o arquivo do checkout usado para publicar. Guarde o hash impresso pela publicação junto dos logs.

## Arquitetura

```text
JSON existente (bytes UTF-8)
    ↓ cliente valida JrSkill v1 e calcula SHA-256
create_skill(schema_version, payload_hash, payload)
    ↓ authority assina e paga rent/taxa
Skill PDA: ["skill", authority, payload_hash] + Program ID
    ↓ RPC + owner + discriminator + desserialização Borsh
cliente verifica authority, versão, hash e igualdade dos bytes
    ↓
artifacts/<PDA>.json (sem execução no robô)
```

Estrutura mínima Anchor de arquivo único em `solana/jrskill-solana`, fixada em Anchor **1.1.2** para acompanhar o CLI já presente no WSL. Cliente JavaScript com `@anchor-lang/core` **1.1.2** e `@solana/web3.js` **1.98.4** (API v1 compatível com Anchor). `Cargo.lock` e `package-lock.json` registram a resolução de dependências. Rust SHA-256 usa `sha2` **0.10.9**.

### Conta Skill

| Campo | Tipo | Responsabilidade |
| --- | --- | --- |
| `authority` | `Pubkey` | Signer que publica e paga a criação |
| `schema_version` | `u8` | Versão do envelope de armazenamento: 1 |
| `payload_hash` | `[u8; 32]` | SHA-256 dos bytes originais |
| `payload` | `Vec<u8>` | JSON original, sem reserialização |

Espaço reservado: `8 + 32 + 1 + 32 + 4 + 512 = 589 bytes`. O overhead inclui discriminator Anchor e prefixo de comprimento Borsh. O limite de **512 bytes** mantém esta prova numa instrução/transação pequena; o cliente verifica também o limite de transação de **1.232 bytes**. Não há protocolo de chunks nesta etapa.

### Regras e limites

- `create_skill` exige signer/payer e PDA derivada pelas seeds esperadas;
- o programa aceita somente `schema_version = 1`, payload não vazio de até 512 bytes e SHA-256 correto;
- `init` permite uma única criação por authority/hash; não existem update, delete, close, registry ou licença;
- o mesmo conteúdo de outra authority gera outra PDA; conteúdo diferente também gera outra PDA;
- não é necessário armazenar bump: a derivação canônica o calcula;
- o programa guarda bytes e valida integridade; **não interpreta JSON nem resolve Recipes**;
- a validação de `{v, run}` e das chamadas v1 existentes fica no cliente. Um cliente alternativo pode publicar bytes que não sejam JrSkill válido; qualquer executor futuro deverá validar novamente;
- imutabilidade refere-se às instruções deste programa. O programa ainda pode ser atualizado pela upgrade authority; esta prova não congela a upgrade authority;
- a referência `recipe("face_sequence")` permanece no JSON, mas a Recipe local não é publicada nem executada nesta etapa;
- scripts de publicação/leitura exigem genesis hash da Devnet; testes on-chain aceitam somente localhost;
- o ID versionado é placeholder. Cada ambiente deve gerar/preservar sua chave e executar `anchor keys sync` antes do build/deploy.

## Plano de teste

Os comandos completos estão em [solana/jrskill-solana/README.md](../solana/jrskill-solana/README.md).

| Prova | Critério | Nível de evidência |
| --- | --- | --- |
| `npm test` | Arquivo preservado, schema v1, comparação e rejeição de alterações | Teste local do cliente |
| `cargo check -p jrskill` | Tipos/macros Rust conferidos no host | Checagem de compilação host; não produz SBF |
| `anchor idl build` + `npm run test:idl` | IDL real codifica instrução/conta, tx cabe e bytes retornam iguais | Compilação host da IDL + teste local do codec |
| `anchor build` | Gera `target/deploy/jrskill.so` e IDL | Compilação SBF |
| `anchor test --validator legacy --provider.cluster localnet` | Cria/lê, rejeita duplicata, hash errado, schema 2, vazio, >512 e PDA incorreta; falha não deixa conta | Validator local; não prova Devnet |
| Deploy Devnet | Program ID executável no cluster correto | Deploy, ainda sem provar publicação de Skill |
| `publish:devnet` | Transação confirmada, PDA e todos os campos iguais ao original | Escrita e leitura on-chain |
| `read:devnet -- <authority>` | Leitura independente, owner/discriminator/hash/versão/authority/bytes corretos | Critério de conclusão da Etapa 2 |
| Repetir `publish:devnet` | Mesma PDA, apenas conferência, nenhuma substituição | Repetibilidade do cliente |

O cliente também rejeita respostas com authority/versão/hash/payload alterados. Os testes do programa exigem validator limpo porque tentam a primeira criação do payload congelado.

## Checkpoint de implementação

Implementados programa, scripts de publicação/leitura e testes. Evidências executadas:

- **Checagem Rust host:** `cargo check -p jrskill` concluído;
- **Compilação da IDL:** `anchor idl build -p jrskill -o target/idl/jrskill.json` concluído;
- **Compilação SBF:** `anchor build` concluído, gerando `.so` e IDL; o ID placeholder difere da chave local de laboratório, por isso o fluxo do usuário exige `anchor keys sync`;
- **Testes locais:** 3 testes de payload e 2 testes do codec/IDL aprovados;
- **Validator local:** 1 teste on-chain aprovado, incluindo criação, leitura e todos os casos negativos da tabela. O `.so` foi carregado explicitamente pelo `solana-test-validator --bpf-program`, usando o ID placeholder e uma wallet descartável. O cliente Node executou no Windows e o validator no WSL, na porta RPC 18899. Não foi executado `anchor test` integrado, pois Node/npm não estavam no PATH do WSL.

**Não houve deploy nem publicação/leitura na Devnet neste checkpoint. Não houve compilação de firmware nem teste físico do JrBot.**

Nenhuma nova função, Recipe, licença ou integração com o executor/painel foi adicionada. A Etapa 3 depende da comprovação deste round-trip na Devnet.

Durante a instalação, `npm audit` reportou avisos em dependências transitivas do stack Anchor/Web3 (toml, stream-json e uuid). O lock foi preservado sem `audit fix --force`; a avaliação/atualização desse stack deve preceder uso em produção. Esta prova permanece isolada de laboratório.

## Referências oficiais

- [Anchor 1.1.2](https://www.anchor-lang.com/docs/updates/release-notes/1-1-2)
- [Cliente Anchor e compatibilidade Web3 v1](https://www.anchor-lang.com/docs/clients/typescript)
- [Account constraints](https://www.anchor-lang.com/docs/references/account-constraints)
- [Account space](https://www.anchor-lang.com/docs/references/space)
- [Solana PDA](https://solana.com/docs/core/pda)
- [Solana transactions](https://solana.com/docs/core/transactions)
- [JrSkill Execution API](JRSKILL_API.md)
