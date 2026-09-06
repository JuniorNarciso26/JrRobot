# JrBot - operar e diagnosticar pelo painel

Painel: **JRBOT-PANEL-V2-02**. Firmware de diagnostico esperado: **JRBOT-V2-DIAG-01**.
Nao e necessario abrir `idf.py monitor` para usar os controles ou ler os resultados.
Esta entrega altera o painel do computador, nao o firmware nem a pinagem.

## Abrir

1. Feche o painel antigo (inclusive sua janela Python) e o monitor Serial, se estiver aberto.
2. Na copia correta da branch, atualize sem descartar arquivos locais:

```bat
cd /d C:\Projetos\JrRobotV2
git pull --ff-only origin v2-revisada
PAINEL.bat
```

O caminho e exemplo: use a copia onde voce trabalha; se usou JrRobot-Diag01, entre nela.
Pare se o pull falhar; nao use reset --hard. Tambem pode abrir PAINEL.bat pelo Explorador.
A janela Python sustenta o painel: mantenha-a aberta. Ela nao e um monitor de comandos.
O navegador abre somente depois que o servidor local consegue iniciar.
Confira JRBOT-PANEL-V2-02 no cabecalho para nao usar um servidor antigo.

3. Selecione **Serial USB**, **COM4** e **Conectar Serial**.
4. O painel abre a porta e consulta o firmware automaticamente. Veja a versao e o perfil.
5. Use **Atualizar estado**, **Testar audio** e **Testar camera**; as respostas ficam na tela.

COM4 e COM6 sao as portas relatadas para esta montagem, nao propriedades universais
nem numeros permanentes do Windows. COM4 = painel/comandos; COM6 = gravacao.
Nao mantenha painel e monitor externo disputando COM4. O painel permite outra porta
se houver renumeracao; escolher COM6 pede confirmacao para evitar confusao.

## O que o painel verifica

Uma porta aberta NAO confirma que o firmware correto esta rodando. O painel espera
resposta com identificador e protocolo 2, e exibe versao, perfil e estado dos modulos.
Um firmware antigo que para no OLED nao vai responder corretamente. Atualizar so o
painel nao substitui esse firmware: ainda e preciso gravar JRBOT-V2-DIAG-01 na COM6.
Use DIAG_V2.bat build e DIAG_V2.bat flash para essa etapa; depois volte ao PAINEL.bat.
**Nao e necessario executar DIAG_V2.bat monitor.**

O estado exibido tem horario de consulta. Clique em Atualizar estado depois de um
reset ou alteracao da placa. O painel nao reenvia testes automaticamente apos timeout.

## Testes e limites

- OLED: no perfil headless_diagnostic fica desabilitado. Os rostos ficam recolhidos,
  mas a comunicacao, audio autorizado e camera autorizada nao dependem da tela.
- Audio: inicia em 10% no painel. Testar audio configura o volume e aguarda o envio
  do tom. Resposta positiva confirma dados I2S enviados, nao som audivel.
- Camera: Testar camera usa camera_test por Serial/COM4, sem abrir monitor externo.
  Informa sensor, dimensoes e bytes do quadro. Nao mostra foto nem ativa autofocus.
  A espera pela resposta e de ate 30 segundos. Nao repete a captura automaticamente.
- Microfone: aparece nao configurado e sem botao de teste ativo. Ainda falta modelo,
  pinagem e driver. O teste do amplificador nao valida o microfone.
- Wi-Fi: comandos usam POST; salvamento de rede e feito pela Serial, exige resposta
  positiva e reinicializacao. A senha nao e gravada no log nem no armazenamento local.

Se audio ou camera aparecem **desabilitados no firmware**, nao e necessario um
monitor para confirmar: o proprio estado do painel explica o bloqueio. As protecoes
CONFIG_JR_AUDIO_PINS_CONFIRMED e CONFIG_JR_CAMERA_PINS_CONFIRMED continuam no firmware.
Ainda exigem conferencia fisica e compilacao com as opcoes adequadas. O painel nao
habilita pinos desconhecidos nem transforma uma protecao de compilacao em uma chave
remota. Consulte DIAGNOSTICO_SEM_OLED.md para a configuracao, nao para operar testes.

Use **Baixar log TXT** para compartilhar as respostas visiveis com o agente.

## Validacao desta alteracao

33 testes Python de transporte, HTTP local, limites, erros e resposta correlacionada.
13 verificacoes de navegador Chromium com rede e dispositivo simulados, incluindo
COM4, firmware antigo, falha sem sucesso falso, camera Serial e layout de 390px.

Reproduzir no computador:

```text
python tests/panel/test_backend.py
python tests/panel/test_browser.py
```

O segundo exige Playwright e Chromium; sao ferramentas de teste, nao dependencias
para usar o painel. Nenhum teste usou ESP32 fisico. O lancador Windows foi revisado,
mas nao executado neste ambiente Linux. Nenhuma compilacao C nova foi feita nesta
entrega: firmware permanece o de f38ba2c. Resultado detalhado: tests/panel/results.json.
