# V2.2 — teste isolado do TensorFlow Lite Micro oficial

Branch: `test/tflite-yes-no`, criada da V2.1 aprovada (`cb3c0048`).

## Instalação e teste

1. Feche o painel/qualquer monitor que esteja usando a porta COM do robô.
2. Execute `INSTALAR.bat` normalmente e escolha `test/tflite-yes-no` no menu.
3. Escolha a porta COM. O instalador baixa a fonte oficial, prepara o exemplo,
   compila, grava e abre o monitor Serial automaticamente.
4. Aguarde `Audio Recording started`. Fale **yes** (iés) e **no** (nôu), em inglês,
   a cerca de 20–40 cm do microfone, deixando dois segundos entre as palavras.
5. Procure `Detected yes, score: ...` ou `Detected no, score: ...`.
   `silence` e `unknown` também são classes do modelo. O limiar original é 0,8.
6. Observe também se aparecem reconhecimentos em silêncio ou com outras palavras.
   Copie o log com tentativas corretas/incorretas para avaliarmos o resultado.
7. Para sair do monitor, use `Ctrl+]`. Para restaurar o robô completo, execute
   novamente `INSTALAR.bat` e escolha `main`.

Este teste substitui temporariamente a aplicação gravada na placa. Não inicia
Wi-Fi, câmera, OLED, WakeNet ou vTTS e não responde pelo alto-falante. A V2.1
continua preservada na `main`; a alteração ainda não é uma integração com ela.
O painel web do robô não estará disponível durante este teste.

No PowerShell, a forma explícita é `.\INSTALAR.bat`. As ações `build`, `flash`,
`menuconfig` e `panel` continuam disponíveis: `flash` grava sem abrir o monitor;
`panel` abre somente o painel de PC (sem os diagnósticos do firmware completo).

## Origem e alterações

- Repositório: https://github.com/espressif/esp-tflite-micro
- Commit fixado: `3d0b4610fffffbf211d6042824d5d4c408266d93`.
- Exemplo: `examples/micro_speech`; licenças originais preservadas.
- O instalador clona a biblioteca oficial em `.tflite-yes-no/upstream` e prepara
  uma cópia do exemplo em `.tflite-yes-no/project`. Tudo isso fica fora do Git.
- O modelo yes/no, o modelo de pré-processamento e a extração de características
  são copiados sem alteração. Não há treinamento novo nem dependência de nuvem
  durante o reconhecimento. A preparação inicial requer internet.
- Hardware HW04: BCLK=21, WS=47, MIC=41, canal esquerdo, 16 kHz, slots de 32 bits.
  GPIO42 fica em zero para manter o amplificador silencioso.
- Adaptações: largura WS correspondente ao microfone já validado no JrBot,
  saturação da conversão PCM (mesmo ganho x4 do upstream), alinhamento de buffers,
  checagem de erros I2S/captura/inicialização, pausa de um tick no loop.
- `JR_MIC` registra periodicamente pico da captura e heap livre. É um retrato
  de um bloco de áudio, não uma medição de precisão nem de consumo total do modelo.
- Mantido o classificador original com argmax/limiar 0,8; pode imprimir a mesma
  palavra várias vezes enquanto ela permanece na janela de áudio.
- Projeto/partições/build separados do firmware principal. Layout do exemplo
  cabe em 4 MB de flash; nenhuma chamada de `erase_flash` é adicionada.
- Usa o ambiente ESP-IDF 6.1 já usado pela V2.1, inclusive sua validação de Python.
  O gerenciador IDF resolve a dependência ESP-NN exigida pelo upstream e guarda
  o resultado em `dependencies.lock` local; essa dependência transitiva não está
  fixada neste repositório. O build/config ficam preservados para recompilações.

## Validação disponível

Preparação executada contra o clone real fixado; testes verificam que os modelos
e o pré-processamento ficam idênticos, a repetição preserva build/config/lock e
um checkout com alterações é recusado sem sobrescrita. Testes existentes do
instalador também foram executados.

Não houve compilação ESP-IDF neste ambiente nem teste físico do reconhecimento.
O primeiro build no Windows e o teste no microfone confirmarão compatibilidade,
memória efetiva e qualidade de reconhecimento. O README do exemplo oficial ainda
contém referências antigas de IDF, embora o código clonado já use o driver I2S novo.
