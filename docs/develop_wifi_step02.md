# develop_wifi: etapa 02 — quatro redes salvas e selecao manual

Versao: `JrBot_V1S_WIFI_STEP02`. Base: STEP01, commit
`f03b561d92544759c54daa177eeaaa23f8aec61e`.

O log 17 mostrou busca de seis pontos de acesso e Live com 400 quadros JPEG.
O log 19 confirmou a recuperacao de CPGAS_5G em DHCP, IP 192.168.0.123, sem
desconexoes depois do ultimo reinicio. Na busca das 16:01, a RAM interna ficou
igual antes/depois: 126979 bytes livres, maior bloco 36864 bytes. O final do
log 19 nao inclui outra sessao de Live; ha erros de handshake HTTPS entre
conexoes atendidas. A troca/forcamento de IP ficou adiada a pedido do usuario.

## Comportamento

- **Consultar redes salvas** lista ate quatro redes, sem mostrar senhas.
- A configuracao anterior e preservada como primeira rede na primeira inclusao.
- **Configurar Wi-Fi no ESP32** guarda a rede e a seleciona para o proximo boot.
- **Guardar rede sem trocar** guarda os dados do formulario sem alterar a rede
  selecionada, a conexao atual ou exigir reinicio.
- Mesmo SSID atualiza a entrada, sem ocupar outra vaga.
- **Usar rede salva** reaproveita senha, DHCP/IP fixo e demais ajustes guardados.
  Reinicie para aplicar. Ainda nao existe selecao automatica por disponibilidade.
- **Excluir rede salva** remove apenas a entrada escolhida. A rede selecionada
  e a rede em uso ficam protegidas; escolha outra e reinicie antes de excluir.
- **Desativar Wi-Fi** limpa a selecao de boot, mantendo o catalogo de redes.
  Usar uma rede salva permite reativar depois.
- Nao altera camera, audio, WebRTC, SDKCONFIG, inicializacao/reconexao ou busca.

Persistencia usa chaves `p0` a `p3` no namespace NVS `jr_wifi`, com o mesmo
registro validado do Wi-Fi aprovado. `cfg_v2` permanece a selecao de boot.
Nao cria tarefas nem guarda um array permanente de credenciais em RAM.
Falha de gravacao/commit e reportada; nao se deve considerar um erro de
persistencia como rollback garantido. Reconsultar/reiniciar permite conferir
o estado gravado, como no protocolo anterior.

## Teste na placa

1. Instalar `develop_wifi` e confirmar `JrBot_V1S_WIFI_STEP02`.
2. Conectar Serial e consultar redes salvas: CPGAS_5G deve aparecer.
3. Informar outra rede real, senha e DHCP; clicar **Guardar rede sem trocar**.
4. Consultar a lista: ambas devem aparecer. Confirmar que CPGAS_5G continua
   conectada e testar foto, Live e audio nos dois sentidos.
5. Selecionar CPGAS_5G na lista e clicar **Usar rede salva**, sem redigitar senha.
   Reiniciar e conferir conexao, persistencia das duas redes e Live novamente.
6. Se a segunda rede estiver disponivel, usar essa rede salva, reiniciar,
   consultar o IP e testar pelo computador/celular conectado a mesma rede.
7. Voltar a CPGAS_5G e reiniciar. Excluir a segunda rede e conferir a lista.
8. Baixar o log. Aprovar esta etapa antes de adicionar troca automatica.

## Verificacao local

- `python tests/wifi_profiles/test_profiles.py`: compila o `jr_wifi.c` real
  contra NVS simulado; testa preservacao do registro anterior, limite de quatro,
  atualizacao, senha reutilizada, reinicio, exclusao protegida, desativacao sem
  apagar redes, entradas invalidas, falha de gravacao/commit e resposta pequena.
- `python tests/wifi_scan/test_native_scan.py`: mantem verificacao da busca.
- `node --test tools/jrbot_frontend/tests/wifi-scan.test.mjs`: busca, formulario,
  selecao por id sem enviar senha novamente, salvar sem trocar e exclusao.
- Python: compilacao sintatica do painel. Compilacao ESP-IDF completa e testes
  no hardware dependem do instalador/placa do usuario.
