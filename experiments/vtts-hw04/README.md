# Laboratorio JrBot V2 - vTTS pt-BR offline no ESP32-S3 HW04

**Branch experimental:** `test/vtts-pt-br-hw04` (separado da `main` V2.0).  
**Estado:** codigo preparatorio publicado, **sem build nem teste fisico nesta branch**.

## Correcao do INSTALAR.bat para troca de branch — 24/11/2026

Se apareceu `starting point origin/test/vtts-pt-br-hw04 is not a branch`, o
clone ainda tinha remote.origin.fetch limitado a develop. O instalador agora
salva o mapeamento de todas as branches antes do fetch, troca usando a ref
completa e ajusta o upstream das branches locais. Continua sem reset automatico
e preserva a verificacao de alteracoes locais e merge fast-forward-only.

Como o runner antigo e copiado antes da atualizacao, primeiro atualize o clone
que permaneceu na develop apos a falha:

```powershell
git pull --ff-only origin develop
.\INSTALAR.bat
```

Selecione `test/vtts-pt-br-hw04` no menu. O instalador desta branch ja encaminha
para VTTS_HW04.bat, mantendo o laboratorio separado do build do firmware completo.
Para a frase padrao, apos instalar e fechar o monitor com Ctrl+], execute
`VTTS_HW04.bat benchmark COM8`, ou abra VTTS_HW04.bat e escolha a opcao 7.
Os testes host reproduzem o erro Git e verificam a correcao; execucao real do
batch no Windows e compilacao/gravacao ainda devem ser verificadas no computador.

## Teste comparativo padrao JR_TTS_LONG_01 — 24/11/2026

Este primeiro video mede **Enter no computador → inicio acustico → fim da fala**
no laboratorio isolado. So depois de analisar esse resultado adaptaremos a develop
para repetir o mesmo texto e ajustes no JrBot completo. Nenhuma otimizacao do motor,
cache, memoria, taxa de audio ou reproducao foi aplicada nesta etapa.

**Frase canonica:** [frase_padrao.txt](frase_padrao.txt), uma unica linha UTF-8:

> Olá! Eu sou o JrBot. Minhas habilidades são ouvir, falar e me expressar. Estou aprendendo a entender você e a responder cada vez melhor. Posso ajudar com perguntas, compartilhar informações e participar das suas ideias. Este é o nosso teste padrão de voz: vou falar esta mensagem inteira, do começo ao fim. Como posso ajudar você hoje?

Identificador: `JR_TTS_LONG_01`. Texto: **344 bytes UTF-8**.
SHA-256 do texto sem quebra de linha final: `69686839186ec4318b1f0acbc08b50f79d73206f70a29183fd09081a95c7ea82`.
Este arquivo sera a fonte da mesma frase na develop; nao redigitar nem simplificar.

### Executar e filmar

No clone que usa **test/vtts-pt-br-hw04**, abra o terminal ESP-IDF 6.1.1.
Se o robo ainda executa o firmware completo, grave o laboratorio e abra o monitor:

```powershell
python experiments/vtts-hw04/run.py all COM8
```

Aguarde `READY`; saia do monitor com **Ctrl+]** para liberar a COM8.
Se o laboratorio ja esta instalado, nao e necessario recompilar para usar o novo
comando no computador. Antes do video, reinicie o robo, aguarde `READY` e feche
qualquer monitor que esteja segurando a porta. Nao fale outras frases nem execute
bench antes desta primeira medicao apos boot.

```powershell
python experiments/vtts-hw04/run.py benchmark COM8
```

O comando consulta o console pt-en e aplica/confirma **Lite S, pt, volume 75,
ganho 6 dB, speed 1.0, expressividade 1.0, debuzz 0, dois nucleos, chunk 128 e
primeiro bloco 12**. Limpa os contadores de `prof`, sem sintetizar uma fala de
aquecimento, e mostra **PRONTO**. Entao:

1. Comece a filmar o teclado/tela e o robo, com o alto-falante audivel.
2. Pressione **Enter uma unica vez**. Aparece `JR_TTS_BENCH_BEGIN` e o programa
   envia o texto fixo completo, sem copiar/colar manualmente.
3. Filme ate acabar a ultima palavra e mantenha mais dois segundos de gravacao.
4. Envie o video e o log salvo em `.work/results/JR_TTS_LONG_01_*.txt`.

Se houver timeout, escrita incompleta, erro de sintese, watchdog ou reinicio,
o comando encerra e preserva o log. Nao reenvia a frase automaticamente.
O terminal Serial upstream nao faz eco do texto digitado; o marcador do programa
ajuda a localizar o disparo no video. O instante da tecla no video e a referencia
principal, e nao a hora em que o log ficou visivel na tela.

### O que os tempos significam

- **Video:** Enter → primeiro som, duracao da fala, pausas e palavras audiveis.
- **SAID/RTF/PROF:** diagnosticos do motor, preservados para descobrir gargalos.
- **HOST_SEND_COMPLETE:** tempo ate enviar a frase pela Serial; nao e inicio do som.
- **JR_TTS_BENCH_DONE:** tempo ate receber DONE; nao certifica o instante acustico final.

O script nao altera nem gera firmware e nao precisa de `idf.py` para a acao
benchmark; requer o `pyserial` do ambiente ESP-IDF. A placa precisa estar com o
laboratorio gravado. Para apenas exibir o texto: `python experiments/vtts-hw04/run.py phrase`.

**Comparabilidade:** o laboratorio original carrega os modelos no boot e usa
`BSP_DAC_UPSAMPLE=2` (22,05 kHz do modelo → saida de 44,1 kHz); o firmware integrado
carrega o modelo sob demanda e reproduz a 22,05 kHz. Essas diferencas ficam
registradas, sem corrigi-las antes da referencia. Mesmo texto/ajustes permitem
comparar o comportamento percebido, mas nao tornam os dois caminhos identicos.
Repetir no mesmo robo/alto-falante, com a mesma alimentacao e primeira fala apos
boot; nao comparar este S com um video M. A main e a develop ficam intactas nesta etapa.

**Checks do novo comando:** quatro testes host validam bytes/ajustes e envio unico
apos Enter, recusa de console diferente, timeout sem reenvio, erro/watchdog e
leitura fragmentada UTF-8; sintaxe Python verificada. A nova acao ainda precisa
ser executada na COM real. Nao foi realizado novo build/flash ou teste acustico aqui.

## O que vamos testar

Digite uma frase em portugues na **UART0 do ESP32-S3 (conector USB com CH343/COM8)**, pressione Enter e
o **vTTS `nu-pt` (portugues brasileiro + ingles)** sintetizara a frase
**inteiramente no ESP32-S3**, reproduzindo a voz pelo **MAX98357A**.

Exemplo:
```text
READY (type a sentence to speak it; `help` lists the commands)
lang pt
model s
vol 75
Ola! Eu sou o JrBot e agora tenho voz.
SAID ...
DONE
```

A frase nao e um WAV pre-gravado. O vTTS converte **texto livre em audio PCM**.
O microfone e o WakeNet **nao fazem parte deste firmware de laboratorio**.
A intencao e validar a sintese *antes* de integrarmos ao firmware completo.

## Adaptacao minima da placa

| Funcao | JrBot HW04 | Uso neste laboratorio |
|---|---|---|
| ESP32-S3 | N16R8 | 16 MB Flash, 8 MB PSRAM octal |
| I2S BCLK | GPIO21 | clock do MAX98357A |
| I2S WS | GPIO47 | word select do MAX98357A |
| I2S DOUT | GPIO42 | entrada DIN do MAX98357A |
| Microfone | GPIO41 | nao inicializado neste teste |
| OLED/camera/Wi-Fi/WebRTC | presentes no robô | nao inicializados |

O overlay de placa em `board/` seleciona o driver
`speaker_i2s_dac.c` **do upstream** do vTTS, com I2S
16-bit mono transmitido em ambos os slots e amostragem do modelo
22,05 kHz reproduzida a 44,1 kHz (interpolacao 2x).
Nao usamos o codec ES8311 nem alteramos a pinagem do JrBot.

## Atualizacao de console para CH343 / COM8 (23/11/2026)

O primeiro build do laboratorio foi concluido no Windows com ESP-IDF 6.1.1:
firmware de cerca de 5,83 MB, cabendo na particao de 8 MB.

**Diagnostico do teste real:** o Windows identificou o adaptador
`USB-Enhanced-SERIAL CH343 (COM8)`. A configuracao inicial do upstream
`sdkconfig.defaults.esp32s3` enviava texto e entrada pelo **USB Serial/JTAG
interno**; por isso a COM8 mostrava mensagens iniciais de boot, mas nao
mostrava `READY` nem recebia frases. O aviso GDB sobre COMx e o
`KeyboardInterrupt` ao sair do monitor nao comprovam falha da aplicacao.

**Correcao nesta branch:** `board/sdkconfig.defaults` seleciona
`CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, desativando a selecao anterior
de USB Serial/JTAG, e utiliza UART0 / 115200 pela ponte CH343. O
`run.py` corrige tambem o `sdkconfig.jrbot-hw04` que ficou salvo no
primeiro build, fazendo backup com sufixo `.before-ch343` antes de
reconfigurar. A versao V2.0 na `main` continua intocada.

### Para quem ja compilou a primeira revisao

No terminal **IDF_PowerShell v6.1.1**, na raiz do clone
`JrBot_VTTS_Test`, execute:

```powershell
git pull --ff-only origin test/vtts-pt-br-hw04
.\VTTS_HW04.bat build
.\VTTS_HW04.bat flash COM8
.\VTTS_HW04.bat monitor COM8
```

O programa pede `GRAVAR` no momento do flash. **E necessario gravar
novamente**: so atualizar o branch ou abrir o monitor nao corrige o
firmware antigo ja gravado. Se o monitor retornar ao PowerShell com
`KeyboardInterrupt`, use preferencialmente **Ctrl+]** para sair.

Verifique se a linha `READY` aparece na COM8; depois execute
`model s`, `lang pt` e uma frase livre. Se continuar sem `READY`,
capture o boot completo da COM8 apos RESET/EN para diagnosticar a
inicializacao separadamente.

## Pre-requisitos no Windows

1. **Git, Python 3.11+ e internet** para baixar o codigo original e os modelos.
2. **ESP-IDF 6.1** instalado **separadamente** do **5.5.x** usado pela V2.0.
   O projeto original exige 6.1; nao troque a instalacao do JrBot principal.
3. Abra o terminal **IDF_PowerShell** referente ao **ESP-IDF 6.1**.
   Se nao o tiver, instale pelo [Espressif Installation Manager](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html).
4. Ligue o JrBot HW04 por USB e identifique sua porta COM.

O launcher tenta localizar tambem instalacoes tradicionais do ESP-IDF 6.1 em
`%USERPROFILE%\esp\v6.1*` e `C:\Espressif\frameworks\esp-idf-v6.1*`.
O modo `doctor` informa se o terminal esta pronto.

## Como executar

Na raiz do repo, apos selecionar este branch com `INSTALAR.bat` ou
`git switch test/vtts-pt-br-hw04`, abra o terminal do ESP-IDF 6.1:

```bat
VTTS_HW04.bat doctor
VTTS_HW04.bat prepare
VTTS_HW04.bat build
VTTS_HW04.bat flash COM8
VTTS_HW04.bat monitor COM8
```

Apenas **prepare** baixa o codigo e os modelos, sem exigir ESP-IDF.
**build** faz prepare + compilacao **sem gravar o hardware**.
**flash** recompila e pede a palavra `GRAVAR` antes de gravar.
**monitor** abre a Serial. Ou execute o fluxo inteiro:

```bat
VTTS_HW04.bat all COM8
```

**`INSTALAR.bat` neste branch encaminha build/flash/all ao launcher
especifico**, impedindo a compilacao acidental da firmware normal.
Substitua `COM8` pela porta correta do seu hardware.

### Comandos no monitor Serial

```text
help
model s
lang pt
vol 75
Ola! Eu sou o JrBot. Como posso ajudar?
A temperatura hoje esta agradavel.
model m
Bom dia! Estou testando minha nova voz.
bench
prof
```

Use tambem frases com acentuacao, como
**`Olá, eu sou o JrBot. Posso falar português!`**.
O monitor envia os bytes UTF-8 via USB-Serial-JTAG.
A saida `SAID` deve mostrar tempo e RTF; `DONE` confirma fim do comando,
**mas a verificacao do som precisa ser auditiva**. Para sair do monitor,
pressione **Ctrl+]**.

## Como o setup funciona

1. `run.py` baixa o projeto original
   [tuanpmt/vtts](https://github.com/tuanpmt/vtts) e fixa o commit
   **`010cd22fd51f58dfa2414824cb8d6345e70cafc6`**.
2. Baixa da release oficial
   [v0.11.0](https://github.com/tuanpmt/vtts/releases/tag/v0.11.0)
   os modelos `vtts-pten-nupt-s-v0.11.q8.vtts` (~1,2 MB) e
   `vtts-pten-nupt-m-v0.11.q8.vtts` (~2,6 MB).
3. Adiciona **somente** o overlay da placa HW04 a copia local do upstream.
4. Gera o dicionario ingles auxiliar (`lex_en.bin`) a partir da fonte
   original e do CMUdict fixado pelo upstream. A primeira geracao pode
   exigir rede e processamento adicional.
5. Compila o **firmware vTTS de referencia** com
   `VTTS_BOARD=jrbot-hw04`, `VTTS_LANGS=pt` e ESP-IDF 6.1.

O codigo externo, modelos, lexicon e resultados vao para
`experiments/vtts-hw04/.work/`, **ignorado pelo Git**. Nao ha pesos
treinados pelo projeto JrBot; nao substituir por arquivos falsos.
Este experimento nao acrescenta dependencias nem novos pesos a `main`.

## Criterios de aceitacao

- [ ] Compilar com ESP-IDF 6.1 e image/particoes que caibam em Flash 16MB.
- [ ] Ouvir frases diferentes, com e sem acentos, no MAX98357A.
- [ ] Comparar modelos **S** (Lite) e **M** (Quality).
- [ ] Verificar `SAID`, `RTF`, falhas de sintetizacao, RAM interna e PSRAM.
- [ ] Tentar ao menos dez frases de duracoes diferentes.
- [ ] Apos teste isolado, restaurar **`main` V2.0** e verificar camera,
  WakeNet, Live, OLED e Wi-Fi antes de planejar integracao.
- [ ] Se aprovado, criar um novo modulo de TTS no firmware principal;
  nao misturar o upstream inteiro ao JrBot prematuramente.

**Aviso de gravacao:** executar **flash** neste laboratorio substitui
temporariamente a firmware V2.0 **no hardware**, mesmo com branches
separados. Para restaurar, selecione a `main` e grave pelo
`INSTALAR.bat` habitual. O repositorio `main` nunca e alterado pelo teste.

## Licencas

O JrBot permanece **MIT**. O codigo e os modelos vTTS v0.11.0 sao
**Apache-2.0**; a copia do upstream mantem `LICENSE` e `NOTICE`
originais. O CMUdict possui seus proprios avisos BSD.
Distribuir um produto com vTTS exigira preservar os avisos aplicaveis,
inclusive de alteracoes. Apache-2.0 nao obriga tornar todas as partes
do JrBot Apache-2.0.

- [vTTS README / licenciamentos](https://github.com/tuanpmt/vtts)
- [Firmware vTTS ESP32-S3](https://github.com/tuanpmt/vtts/tree/main/firmware/esp32)
- [JrBot V2.0 / main](https://github.com/JuniorNarciso26/JrRobot/tree/main)
