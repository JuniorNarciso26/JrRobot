# Etapa 4A — selecionar e conectar carteira no painel

## Checkpoint — 02/10/2026

Decisao do usuario: testar a carteira no painel antes da implementacao de compra. Esta prova prepara a interacao com a carteira; nao amplia o escopo comercial 00 da [Issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46).

Painel: **JRBOT-PANEL-V1S-WALLET-04**, somente na `V1s-00`. Firmware, contrato, JSON v1 e Recipes preservados.

Implementado:

- Descoberta de extensoes Wallet Standard com suporte anunciado a Solana Devnet.
- Escolha da extensao e solicitacao de conexao apenas por clique.
- Endereco publico conectado, copiar endereco e desconectar.
- Se a extensao autoriza varias contas, escolha entre as contas autorizadas; se autoriza apenas uma, a troca acontece na propria extensao.
- Eventos de troca/remocao de conta e remocao da extensao atualizam ou limpam o endereco.
- Recusa e erros de conexao tratados sem marcar a carteira como conectada.
- Sem extensao, instrucoes e link oficial para instalar Phantom.

**Conectada nao significa autenticada ou licenciada.** Nao ha assinatura de mensagem, compra, listagem de licencas ou autorizacao de execucao implementada nesta prova. As execucoes experimentais local/Devnet existentes continuam independentes da carteira, claramente identificadas no painel.

O indicador "Ambiente do painel: Solana Devnet" identifica nosso ambiente de teste, nao comprova a selecao de rede na interface da extensao. Conectar nao consulta saldo nem envia transacao. A configuracao de testnet/Devnet da carteira sera necessaria para as proximas provas com transacoes.

## Teste no computador do usuario

1. Instalar a extensao pelo [site oficial da Phantom](https://phantom.com/download) no Chrome e criar uma nova carteira de teste. Guardar a frase de recuperacao na propria rotina privada; nao enviar ao painel, ao chat ou ao GitHub.
2. Fechar o painel anterior. Atualizar exclusivamente a `V1s-00`, usando `INSTALAR.bat panel` e selecionando essa branch. Esse modo abre o painel sem compilacao/gravação do firmware.
3. Abrir `http://127.0.0.1:8765` no mesmo Chrome em que a extensao esta instalada. Se o instalador abriu outro navegador, copiar esse endereco para o Chrome.
4. Confirmar a versao `JRBOT-PANEL-V1S-WALLET-04`. Usar Ctrl+Shift+R se necessario.
5. No bloco Carteira JrSkill, escolher Phantom e clicar Conectar carteira. Aprovar o acesso na extensao.
6. Conferir que o endereco do painel corresponde ao endereco **Solana** selecionado na carteira. A mensagem deve indicar conectada e nao autenticada.
7. Desconectar e confirmar que o endereco some. Reconectar; depois trocar a conta autorizada pela extensao e verificar a atualizacao. Se a extensao nao emite a troca, desconectar/reconectar para autorizar a nova conta.
8. Testar recusar uma nova solicitacao de conexao. O painel nao deve exibir endereco conectado nessa tentativa.
9. Baixar o log TXT do painel, que inclui eventos `JR_WALLET`, e enviar o resultado do teste.

Esta prova nao precisa de saldo/SOL, robo ligado ou acesso a carteira administrativa usada no deploy. Nao importar o arquivo de chave do programa ou a carteira administrativa para executar este roteiro.

Se nao detectar a extensao: confirmar instalacao/desbloqueio no mesmo perfil Chrome, atualizar carteiras e recarregar a pagina. A prova inicial e no PC com extensao, sem conexao via app de celular/QR code. Outras marcas precisam anunciar as funcionalidades padrao exigidas e ser testadas; nenhuma compatibilidade universal e alegada.

## Validacao realizada

- Bundle gerado com esbuild e verificado com `node --check`.
- 7 testes JavaScript do controlador passaram: descoberta tardia/filtro, conexao explicita, troca/revogacao de conta, troca de provedor, recusa/retry, falha de desconexao e remocao durante conexao/duplo clique.
- 3 testes existentes do executor Devnet passaram.
- 6 testes Python existentes de integridade/rotas Solana passaram.
- 1 teste Python passou servindo HTML e bundle pelo encadeamento real de wrappers do `run_panel_network.py`, usado pelo PAINEL.bat.
- Teste de interface no Chrome headless com extensao **simulada**: sem extensao, registro tardio via Wallet Standard, conexao por clique, troca de conta e desconexao; sem erros JavaScript. Nenhuma carteira real ou transacao foi usada nesse teste.

**Ainda pendente:** conexao/aprovacao/troca de conta com extensao real instalada pelo usuario. Nao alegar validacao fisica, autenticacao criptografica, compra ou deploy por estes testes.

## Proximas provas

Depois da conexao real: discutir e implementar autenticacao com desafio assinado/verificado, seguida da compra isolada do modelo 00 e consulta de licencas. Conexao de carteira sozinha nao autoriza uma Skill comercial.

Referencias: [Wallet Standard](https://github.com/wallet-standard/wallet-standard), [Phantom — integracao Solana](https://docs.phantom.com/solana/integrating-phantom), [Etapa 4](JRSKILL_SOLANA_STAGE4.md).
