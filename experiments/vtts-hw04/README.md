# Laboratorio JrBot V2 - vTTS pt-BR offline no ESP32-S3 HW04

**Branch experimental:** `test/vtts-pt-br-hw04` (separado da `main` V2.0).  
**Estado:** codigo preparatorio publicado, **sem build nem teste fisico nesta branch**.

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
