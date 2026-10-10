# Etapa 4B — autenticar carteira por assinatura

## Implementacao — 02/10/2026

Painel **JRBOT-PANEL-V1S-WALLET-05**, somente na `V1s-00`. Conexao e autenticacao sao etapas separadas. Esta entrega nao faz compra, cria licenca ou concede autorizacao de executar uma Skill comercial.

Atualizacao: o log (73) do usuario confirmou autenticacao em 02/10/2026. A versao seguinte WALLET-06 adiciona consulta Devnet/saldo; [evidencia e roteiro da Etapa 4C](JRSKILL_WALLET_DEVNET_TEST.md). Os limites/testes descritos abaixo sao o checkpoint original da 05.

Fluxo por clique:

1. Carteira conectada; selecionar **Autenticar carteira**.
2. Servidor local gera um nonce aleatorio e armazena o desafio associado ao cookie da sessao, endereco da carteira e origem local exata.
3. A mensagem UTF-8 contem origem, endereco, ambiente Devnet, nonce, emissao, expiracao e finalidade: sessao local, sem compra ou transferencia.
4. Extensao solicita aprovacao via Wallet Standard `solana:signMessage`.
5. Cliente exige assinatura Ed25519 e mensagem identica ao desafio. Envia ID e assinatura publica em base64 ao servidor local.
6. Servidor verifica Ed25519 sobre a mensagem original armazenada, usando a chave publica decodificada do endereco Solana. O backend, nao uma flag enviada pelo navegador, decide o resultado.
7. Somente a resposta valida para o mesmo endereco muda o indicador para **Conectada — autenticada**.

Verificacao usa `cryptography` (versao de teste/pin: 50.0.1), sem implementar algoritmos criptograficos proprios. O PAINEL.bat verifica se o modulo Ed25519 esta instalado junto com pyserial; se faltar, instala requirements.txt. Servidor nao importa/le carteiras privadas ou arquivos do deploy.

## Sessao e limites

- Desafio: validade de 120 segundos, uma tentativa; substituicao invalida o anterior. Assinatura invalida tambem consome o desafio.
- Sessao autenticada: 600 segundos, sem renovacao automatica. Nova autenticacao exige nova assinatura.
- Cookie aleatorio `HttpOnly; SameSite=Strict`, restrito a `/jrskill/wallet`. Aplicacao servida somente em HTTP loopback (localhost/127.0.0.1); este desenho nao e um login pronto para hospedagem publica.
- POST exige Host local, Origin igual ao Host e cabecalho do painel. Mensagem/desafio e sessao sao vinculados a essa origem, incluindo porta.
- Estado somente em memoria, limite de 256 sessoes e limpeza de expiradas; reiniciar o servidor encerra as sessoes.
- Troca de conta/extensao, remocao ou desconexao limpa imediatamente o indicador e solicita revogacao no servidor. Respostas atrasadas de uma assinatura de outra conta sao descartadas.
- Se o servidor estiver inacessivel, a revogacao remota nao pode ser confirmada: o cliente perde a autenticacao e o registro do servidor expira pelo prazo de dez minutos. Nao se considera uma sessao valida apenas pelo estado visual ou pelo endereco conectado.
- Cliente consulta estado a cada 10 segundos enquanto autenticado e ao recuperar foco. Falha/expiracao/revogacao remove o estado autenticado. Backend aplica a expiracao independentemente do intervalo da interface.
- Abas do mesmo perfil/origem compartilham cookie: desconectar/trocar/autenticar em outra aba pode invalidar a sessao anterior. Nao ha persistencia de autenticacao em localStorage.
- Logs contem eventos/endereco publico, nunca nonce, assinatura ou token de sessao. O identificador do desafio e descartado apos a tentativa.

Rotas locais: POST `/jrskill/wallet/challenge`, POST `/jrskill/wallet/verify`, GET `/jrskill/wallet/status` e POST `/jrskill/wallet/logout`.

Licencas e compra continuam para as etapas seguintes (#33/#46). A execucao experimental local/Devnet existente permanece independente dessa sessao. Futuras rotas licenciadas deverao consultar a sessao no servidor e validar a licenca da carteira correspondente antes da execucao; nao confiar no indicador visual.

## Roteiro do usuario

1. Fechar painel antigo, atualizar a `V1s-00` e executar `INSTALAR.bat panel`. A instalacao da nova dependencia pode precisar de Internet.
2. Abrir `http://127.0.0.1:8765` no mesmo Chrome da extensao e confirmar `JRBOT-PANEL-V1S-WALLET-05`; Ctrl+Shift+R se necessario.
3. Conectar a carteira nova de teste. O estado inicial continua **nao autenticada**.
4. Clicar **Autenticar carteira** e conferir a mensagem JrBot com origem/endereco corretos e aviso de que nao autoriza compra/transferencia. Aprovar na extensao.
5. Confirmar **Conectada — autenticada**, baixar log TXT e registrar resultado. Nao precisa de SOL ou robo ligado.
6. Desconectar/reconectar: deve voltar a **nao autenticada**. Testar recusar a assinatura: continua conectada, sem autenticacao.
7. Se possivel, trocar a conta na extensao e conferir que precisa autenticar novamente. Apos dez minutos, tambem deve exigir nova assinatura.

Nao enviar frase de recuperacao ou chave privada. Extensao que nao oferece `solana:signMessage` pode conectar, mas nao autenticar nesta prova. Assinaturas com prefixo/mensagem diferente sao recusadas deliberadamente.

## Validacao realizada e pendencias

- 6 testes Python de autenticacao passaram: assinatura real Ed25519, expiracao, outra chave/mensagem, replay, origem/sessao distinta, revogacao/substituicao, endereco invalido, limite de sessoes e fluxo HTTP/cookie pelo mesmo encadeamento do PAINEL.bat.
- 4 testes JS de autenticacao passaram: acao explicita, recusa/mensagem alterada, troca de conta durante assinatura, resposta para outra carteira e revogacao do servidor.
- Os 17 testes anteriores de conexao, rotas, integridade e executor tambem passaram: 27 testes automatizados ao todo neste checkpoint.
- Interface Chrome headless com carteira **simulada** gerou uma chave efemera via WebCrypto, assinou o desafio e foi verificada pelo servidor Python real. Conexao, autenticacao, revogacao por troca de conta, recusa/retry e desconexao passaram, sem erros JS ou transacao Solana. Chave efemera nao foi salva.

**Teste do usuario recebido:** log (73) registra conexao e autenticacao confirmada pelo servidor. Recusa, troca de conta e expiracao com extensao real continuam sem evidencia nesse arquivo. Esta entrega nao representa teste fisico do robo, compra, compilacao de firmware/programa ou upgrade/deploy Solana.

Referencias oficiais: [Wallet Standard signMessage](https://github.com/solana-labs/wallet-standard/blob/master/packages/core/features/src/signMessage.ts), [cryptography Ed25519](https://cryptography.io/en/latest/hazmat/primitives/asymmetric/ed25519/).
