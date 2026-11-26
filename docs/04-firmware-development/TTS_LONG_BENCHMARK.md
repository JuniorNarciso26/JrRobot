# JR_TTS_LONG_01 — referencia isolada e comparacao integrada

## Investigacao HTTPS — medicao por etapa, sem mudar o firmware

A biblioteca urllib fecha cada conexao apos a resposta (Connection: close).
O benchmark original refaz resolucao de endereco, TCP e TLS em cada consulta;
o log Serial mostra repetidas negociacoes. Isso e um fator confirmado, nao
uma prova da causa unica dos ACKs variaveis de 5 a 15 segundos.

Atualizar ferramentas usando `INSTALAR.bat panel` (atualiza sem compilar/gravar),
ou pelo fluxo habitual, e executar o mesmo `python tools/tts_benchmark.py
https://jrbot.local --insecure-local`. Manter o firmware V2.1.8D instalado,
mesma frase/modelo/volume/ganho e hold de 2 segundos. Nao e preciso regravar.
O log novo contem `BENCHMARK_PROTOCOL=jr-tts-https-phases-v1`.

`HTTPS_PHASES` registra cada action (status/configure/speak), sequence, dns_ms,
tcp_ms, tls_ms, connect_ms, http_headers_ms, body_ms e total_ms. Em erro, campos
posteriores a etapa com falha podem estar ausentes. proxy_tunnel=true indica
que tls_plus_proxy_ms inclui CONNECT do proxy. Nao registrar credenciais ou
configuracao de proxy. O intervalo http_headers_ms inclui envio do pedido e
espera dos cabecalhos, nao e exclusivamente CPU do handler do robô.

O cliente mantem transporte original, sem reuso de conexao nem cache proprio de
DNS, para medir a causa antes de modificar comportamento. Tentativas TCP em
enderecos alternativos acontecem antes de enviar HTTP. Nao seguir redirects,
nao desativar validacao TLS sem --insecure-local e nunca repetir POSTs apos
entrega incerta. TLS e medido mantendo SNI/validacao da biblioteca padrao.
Se DNS dominar, investigar mDNS/enderecos; se TLS dominar, avaliar reuso de
sessao/conexao; se a espera HTTP dominar, medir handler e agendamento do servidor.
Nenhuma dessas hipoteses foi demonstrada apenas pelos logs antigos.

## Segunda rodada — V2.1.8D, reserva inicial de 2 segundos

Atualizar develop por INSTALAR.bat e confirmar `JrBot_V2.1.8D_TTS_TWO_BLOCK_2S`.
Usar o mesmo tools/tts_benchmark.py, texto, ajustes e coleta de video com audio
mais log Serial/painel. Enviar uma unica vez, com Live/Autonomo desligados.
Somente `JR_TTS_TWO_BLOCK_HOLD_MS` muda de 4000 para 2000; nenhuma mudanca HTTPS.

Base enviada em 26/11: JR_TTS_LONG_01_develop_20261126T144030957641Z.txt,
JrBot-painel-log (56).txt e IMG_0414.mp4. Versao V2.1.7D, git 04737ccf.
Primeiro bloco: geracao 6065 ms, PCM 7382 ms. Segundo: geracao 8128 ms,
PCM 10214 ms. Margem na fronteira 3255 ms, waited=0. API: synth=14194 ms,
first_i2s=10259 ms, total=27908 ms, RTF=0.807, ESP_OK, utterances=1.
O Wi-Fi tinha uma desconexao historica AUTH_FAIL; o contador permaneceu em 1
no periodo da fala. Nao atribuir esse evento anterior como causa da latencia.

A margem esperada com hold menor e ~1255 ms se os tempos se repetirem.
Aceitar o teste pelo resultado medido: hold_ms=2000, waited=0, margem positiva,
ESP_OK e continuidade sonora confirmada pelo usuario/video. Caso haja espera,
registrar o atraso sem reduzir mais a reserva. O ACK HTTPS de 15100.7 ms e
host_observed=45562.6 ms incluem custos externos aos marcadores do dispositivo;
nao anunciar 10.259 s como tecla-ate-som. Investigar transporte em outra rodada.

## Teste em duas partes — V2.1.7D, 26/11/2026

Referencia aprovada pelo usuario: V2.1.6D, git 1800282b, geracao 21,390 s,
primeiro envio I2S 22,194 s, PCM 17,597 s, ESP_OK. O usuario confirmou audio bom;
o video enviado nao possui faixa de audio. Os ~3,8 s entre geracao total e PCM
nao determinam exatamente a reserva de cada metade sob carga concorrente.

O mesmo comando/mesma frase/mesmos ajustes agora ativam, por igualdade exata
do texto manual, o experimento em duas partes. Corte apos `cada vez melhor.`;
a segunda comeca com `Posso ajudar`. O tamanho em segundos sera medido, nao
fixado pelo numero de caracteres. Outras frases e cache WakeNet seguem o caminho
anterior. A primeira parte fica pronta; uma tarefa de reproducao aguarda 4000 ms
enquanto o produtor gera a segunda; depois toca a primeira e, quando disponivel,
a segunda. Uma unica inferencia usa modelo, arena e normalizador por vez.

As partes ocupam regioes disjuntas da mesma alocacao PCM limitada. O player
mantem I2S, filtro e estado de ganho entre elas. A segunda parte inclui o mesmo
intervalo de 100 ms usado nas demais separacoes do JrBot. Nao ha drenagem ou
reabertura de I2S na fronteira; a drenagem final permanece. Se a segunda parte
falhar, a primeira pode ja ter sido reproduzida: o comando termina em erro,
sem reproduzir PCM incompleto da segunda. O produtor sempre espera a tarefa
de reproducao encerrar antes de liberar memoria e a posse I2S.

Coletar benchmark e log Serial. `JR_TTS_TWO_BLOCK` registra geracao/duracao por
parte. `JR_TTS_TWO_BOUNDARY ready_margin_ms` positivo indica segunda parte pronta
antes do fim do envio da primeira ao I2S; negativo indica atraso.
`waited=1` e `last_underruns` contam espera na fronteira. O DMA ainda pode conter
amostras: essa espera nao e uma medicao exata de silencio no alto-falante.
O video precisa conter audio para avaliar a continuidade acustica.
`last_synth_ms` soma as duas geracoes, excluindo a espera final pela reproducao;
`last_total_ms` inclui hold, geracao e reproducao sobrepostas.

O hold de 4000 ms e um parametro de compilacao (`JR_TTS_TWO_BLOCK_HOLD_MS`),
nao altera velocidade da voz nem garante ausencia de cortes. Nao equivale a
prebuffer adaptativo para mensagens arbitrariamente longas. Build ESP-IDF,
watchdog e desempenho fisico ainda precisam ser validados no HW04.

### Como o vTTS original faz streaming

Fontes primarias no commit pinado 010cd22fd51f58dfa2414824cb8d6345e70cafc6:
[firmware README](https://github.com/tuanpmt/vtts/blob/010cd22fd51f58dfa2414824cb8d6345e70cafc6/firmware/esp32/README.md),
[speaker.c](https://github.com/tuanpmt/vtts/blob/010cd22fd51f58dfa2414824cb8d6345e70cafc6/firmware/esp32/main/speaker.c),
[synth.c](https://github.com/tuanpmt/vtts/blob/010cd22fd51f58dfa2414824cb8d6345e70cafc6/firmware/esp32/main/synth.c),
[vtts_synth.c](https://github.com/tuanpmt/vtts/blob/010cd22fd51f58dfa2414824cb8d6345e70cafc6/runtime/src/vtts_synth.c).

O frontend divide texto por frases e regras do idioma. `stream_run` calcula
duracoes e representacao de tokens, depois gera janelas temporais mantendo
contexto entre elas. O primeiro bloco e pequeno; o tamanho dobra ate o teto
(`first=12`, `chunk=128` no wrapper atual: 12, 24, 48, 96, 128, ...).
Quadros sao unidades temporais internas; sua duracao depende de hop/sample_rate.
O callback PCM alimenta uma fila consumida pela tarefa de audio. O firmware
original insere 150 ms entre segmentos de texto e calcula prebuffer/bloco pelo
RTF medido com fator 1,25 e folga de 50 ms na simulacao do agendamento.
Se a geracao ficar para tras, conta underrun e espera dados; nao garante fala
infinita sem pausas quando a geracao sustentada e mais lenta que a reproducao.

## Referencia recebida em 24/11/2026

Fonte: `JR_TTS_LONG_01_20261124T175649130038Z.txt` e
`WhatsApp Video 2026-11-24 at 14.57.40.mp4` enviados pelo usuario.
Frase canonica em `tools/tts_benchmark_phrase.txt`, 344 bytes UTF-8, SHA-256
`69686839186ec4318b1f0acbc08b50f79d73206f70a29183fd09081a95c7ea82`.
Modelo Lite S, pt, volume 75, ganho 6 dB, speed 1.0, expressividade 1.0,
debuzz 0, dois nucleos, limite de bloco 128 e primeiro bloco 12.
Upstream pinado: `010cd22fd51f58dfa2414824cb8d6345e70cafc6`.

| Medida | Valor | Significado |
| --- | ---: | --- |
| Envio Serial no host | 25,6 ms | Ate concluir o envio; nao mede som |
| Primeiro bloco | 96 ms | Marcador interno do motor |
| Inicio da reproducao | 170 ms | Marcador interno; nao e deteccao acustica |
| Duracao PCM | 17,90 s | 7 segmentos nativos de texto |
| Fila vazia durante fala | 0 | Zero underruns registrados |
| RTF efetivo | 0,322 | Geracao mais rapida que reproducao, ~3,1x |
| Prebuffer calculado | 418 ms | Reserva configurada para proximo comando |
| Conclusao observada no host | 18.085,1 ms | Ate receber DONE, inclui transporte |
| CPU durante fala | 33% / 22% | Cores 0 / 1, valores informados pelo motor |
| Arena | 1.956.688 bytes | PSRAM |

`SAID synth=14.010s total=14.029s RTF=0.784` inclui a espera do produtor por
espaco na FIFO enquanto o som ja toca. O relatorio seguinte desconta o bloqueio:
`rtf 0.322`. Para esta fala, isso corresponde a aproximadamente 5,76 s de
sintese efetiva espalhados durante a reproducao. Nao interpretar 14 s como
latencia inicial. `prebuffer 418 ms` e planejamento do proximo comando, nao
substitui o inicio medido de 170 ms neste comando.

O video mostra tela/robo e permite avaliar as pausas acusticas, mas a tecla Enter
nao fica visivel. Portanto nao atribuir uma latencia tecla→som exata em
milissegundos a partir deste video; 170 ms e um dado interno do firmware.
A gravacao e compativel com inicio rapido e tem pausas curtas, sem o padrao de
intervalos de aproximadamente 2,5 s do teste V2.1.4D anterior. Ausencia de
underruns nao certifica pronuncia perfeita ou cada palavra.

O log confirma o caminho de streaming (primeiro bloco, inicio e underruns),
embora o overlay historico tenha BSP_DAC_UPSAMPLE=2. A taxa efetiva I2S nao esta
neste log: nao deduzi-la somente desse define. Nao houve medicao nova de memoria
interna/fast scratch no log recebido.

**Conclusao limitada a esta amostra:** o HW04 consegue gerar e reproduzir esta
frase longa continuamente no laboratorio. A integracao lenta nao deve ser
atribuida como limite inevitavel do chip; precisamos compara-la nas mesmas
condicoes antes de aplicar otimizacoes.

## Segunda medicao: develop sem alterar o algoritmo

A develop continua com V2.1.5D_TTS_BUFFERED. Nesta etapa so foram adicionados o
emissor do teste, frase fixa, este protocolo e exclusao dos logs locais. O codigo
de sintese/reproducao e o instalador permanecem inalterados.

1. Atualize/instale **develop** pelo INSTALAR.bat. Confirme versao/SHA no boot.
2. Reinicie, deixe Autonomo e Live desligados e nao envie outra frase antes.
   Aguarde Wi-Fi e HTTPS estarem prontos. O painel desktop/Serial pode ser fechado.
3. No terminal da pasta do projeto, execute:

```powershell
python tools/tts_benchmark.py https://jrbot.local --insecure-local
```

Se mDNS nao resolver, substitua jrbot.local pelo IP privado atual, por exemplo
`https://192.168.0.123`. O flag permite o certificado autoassinado exclusivamente
para este teste local; o comando recusa destinos HTTP/publicos e redirecionamentos.
Se o certificado ja for confiavel no computador, omita o flag.

O comando consulta `/voice`, exige I2S livre e TTS disponivel, configura e
confirma os mesmos ajustes de voz da referencia e espera **PRONTO**. Nenhum audio
de aquecimento e sintetizado. Filme a tela/tecla e o robo, pressione Enter uma
vez e grave ate dois segundos apos o final da fala. O texto e enviado exatamente
uma vez por POST `/voice`, sem reenvio automatico em caso de resposta incerta.
Enviaremos tambem `X-JrBot-Voice: 1`, como o painel HTTPS existente.

Envie o video e o log em `.tts-benchmark-results/`. O log preserva o hash da frase,
parametros, tempo do ACK e os campos last_first_audio_ms, last_synth_ms,
last_audio_ms, last_rtf e last_total_ms da API. A consulta ocorre a cada 0,5 s:
`host_observed_ms` tem atraso de polling e HTTPS e nao mede o instante acustico.
`last_underruns=0` na implementacao bufferizada e um campo legado, nao prova
fisica independente de ausencia de cortes. O video e o criterio para continuidade.

**Controle de comparacao:** mesmo robo, alimentacao, alto-falante, texto, modelo e
ajustes, primeira fala apos boot. A referencia usa Serial e modelos pre-carregados;
a develop usa HTTPS/Wi-Fi e carrega o modelo sob demanda. O video mede a espera
percebida com esses custos; os marcadores internos ajudam a separar a geracao.
Nao afirmar que os dois transportes e estados de memoria sao identicos.

## Validacao do emissor integrado

Quatro testes host verificam frase/hash, ajustes antes do Enter e envio unico,
recusa de audio ocupado, destino HTTPS local e ausencia de reenvio na entrega
incerta. O teste existente de sintese bufferizada tambem passou. Sintaxe Python
verificada. Isso nao substitui teste no HTTPS real/ESP32. Nao foi realizada nova
compilacao, gravacao ou medicao acustica da develop nesta etapa.

## Resultado integrado recebido em 24/11/2026, 15:38 (SP)

Fontes: `JR_TTS_LONG_01_develop_20261124T183433482983Z.txt`,
`JrBot-painel-log (54).txt`, `JrBot-memoria-1795545446554.json` e
`WhatsApp Video 2026-11-24 at 15.38.14.mp4`.
Boot/painel confirmam V2.1.5D_TTS_BUFFERED, git b0749963. Frase/hash e ajustes
coincidem com a referencia isolada. O log integrado e a segunda fala do painel
(utterances=2); a primeira tambem levou aproximadamente 134 s para gerar audio.
Portanto esta amostra integrada nao e primeira fala apos boot, como o protocolo
propunha; mesmo assim a lentidao se repetiu, sem melhora relevante na segunda.

| Medida | Isolado | Integrado |
| --- | ---: | ---: |
| Inicio interno da reproducao | 170 ms | 133.291 ms |
| Duracao PCM | 17,900 s | 17,597 s |
| RTF efetivo de geracao | 0,322 | 7,564 |
| Sintese, sem espera da fila isolada | ~5,76 s | 133,097 s |
| Conclusao observada no host | 18,085 s | 153,052 s |

No integrado, ACK HTTPS veio em 1.427,4 ms. A demora principal esta no intervalo
medido pela sintese e nao na confirmacao do envio. Geracao integrada ficou
aproximadamente 23,5 vezes mais lenta por segundo de audio nesta comparacao,
com transportes/estados de memoria diferentes ja documentados. A primeira
fala registrada no painel gerou 17,597 s em 134,456 s (RTF 7,641), repetindo
praticamente o comportamento da segunda (133,097 s, RTF 7,564).

Os sete segmentos e a diferenca de aproximadamente 0,3 s de PCM sao compativeis
com o intervalo entre segmentos: 100 ms no wrapper integrado contra 150 ms no
upstream, seis separacoes. Isso nao e evidencia de perda de palavras.

Durante a sintese, a memoria registra ~2,86 MiB de PSRAM livre e ~129 KiB de
RAM interna livre, maior bloco interno 58 KiB. Apos liberar a fala, PSRAM livre
retorna a ~7,90 MiB, estavel nas consultas seguintes. Nao houve falha de alocacao
ou reset nestas amostras; os valores nao demonstram um vazamento acumulativo.
DMA sobrepoe RAM interna: nao somar os dois totais. Alguns pontos rotulados
`antes` no JSON foram coletados durante a fala; usar timestamps/uptime para
classificar as fases, nao apenas esse rotulo.

### Diferencas identificadas e limites da conclusao

- O wrapper nao chama vtts_set_fast_scratch, ao contrario do laboratorio.
- A sintese manual da V2.1.5D foi colocada em tskIDLE_PRIORITY para permitir
  IDLE0 executar entre chamadas longas do motor. Isso pode contribuir para a
  lentidao; nao atribuir os 133 s somente a prioridade sem medicao por fase.
- O projeto nao explicita as mesmas opcoes de cache do laboratorio nos defaults.
  A configuracao efetiva pode vir de sdkconfig antigo: ler o build existente.
- Estao listadas fontes SIMD/FreeRTOS no CMake, mas isso nao prova quais simbolos
  strong/weak acabaram selecionados no ELF instalado nem quais flags foram usadas.
- Falta perfil por etapa no integrado. Nao concluir que a placa e incapaz nem
  alterar varias configuracoes simultaneamente sem identificar essas diferencas.

### Diagnostico do build existente, sem regravar

Foi adicionado tools/tts_build_diagnostics.py, somente leitura, sem dependencia
externa e sem executar compilador, linker ou comandos na placa. Atualize apenas
o Git local e execute no diretorio do projeto:

```powershell
git pull --ff-only origin develop
python tools/tts_build_diagnostics.py
```

**Nao execute INSTALAR.bat para esta coleta.** Queremos o ELF que ja gerou o
firmware instalado, e nao um build novo. O padrao procura firmware/b-develop-vtts;
se a pasta for diferente, use `--build-dir caminho-da-pasta`.
Envie o JSON indicado em `.tts-benchmark-results/tts-build-*.json`.

O relatorio extrai bindings GLOBAL/WEAK e tamanhos dos simbolos relevantes,
opcoes selecionadas de config/sdkconfig.h e sdkconfig, flags -O e defines VTTS
por unidade compilada, hash do ELF e stamp Git local. Filtra os campos em vez de
exportar sdkconfig/headers completos, que podem conter dados locais. GLOBAL
comprova definicao forte incluida, mas nao comprova ganho, chamadas SIMD em todos
os caminhos ou que o ELF ainda corresponde a placa. Conferir stamp e boot b0749963.
Simbolos ausentes podem ser removidos pelo linker e nao provam falha sozinhos.

Tres testes de estrutura ELF32, bindings e filtragem passaram, junto aos cinco
testes existentes de sintese/emissor. A coleta ainda precisa rodar no build real.
Nenhum algoritmo de firmware, prioridade, memoria ou reproducao foi alterado
nesta etapa de diagnostico.


## ELF inspecionado em 25/11/2026 e correcao V2.1.6D

O usuario executou o leitor no build antigo em Desktop/JrBot_Develop_Voz e
forneceu a saida: dot WEAK/FUNC/45 bytes, dots WEAK/FUNC/35 bytes,
par_run WEAK/FUNC/14 bytes, s3_kernel ausente. O JSON completo ainda nao foi
fornecido. Isso confirma que **esse ELF** selecionou os fallbacks genericos;
nao determina sozinho todas as configuracoes, identidade do flash nem a parcela
exata dos 133 s atribuivel a cada diferenca.

A develop agora identifica-se como V2.1.6D_TTS_ACCEL e exclui o arquivo de tres
fallbacks weak da lista de fontes. O link deve selecionar as implementacoes
fortes do port ESP32-S3 e FreeRTOS; se faltar uma, deve falhar. O verificador
check_tts_acceleration.py rejeita no pos-link um ELF nao Xtensa, funcoes weak,
sem tamanho ou sem tipo FUNC, ausencia de dots/par_run e do objeto vtts_s3_kernel.
Em 26/11, o JSON do build 09b64712 mostrou dots GLOBAL/FUNC/322 bytes,
par_run GLOBAL/FUNC/158 bytes e s3_kernel GLOBAL/OBJECT/4 bytes, mas dot ausente.
Isso e compativel com eliminacao da funcao sem uso por otimizacao/link; nao e
evidencia de fallback. O verificador foi corrigido para aceitar apenas a ausencia
de dot, mantendo sua validacao quando presente e todas as outras exigencias.
O relatorio confirma -O2 apos -Og nos arquivos TTS, CPU 240 MHz e dual-core;
nao mede desempenho nem confirma qual firmware esta gravado no robo.
O instalador tambem verifica depois do build/inventario, antes de anunciar
sucesso ou gravar. Isso cobre um ELF incremental preexistente que nao foi
relinkado. Arquivos upstream nao foram modificados.

Nesta rodada nao mudar prioridade, fast scratch, cache, divisao de texto nem
politica bufferizada. Queremos medir uma causa confirmada de cada vez.
Portanto nao esperar que essa versao ja use o inicio por streaming do laboratorio.

**Proximo teste autorizado:** atualizar/instalar develop por INSTALAR.bat,
confirmar V2.1.6D_TTS_ACCEL e SHA no boot, e verificar a linha
`JR_TTS_ACCELERATION_CHECK` com sucesso no instalador. Repetir a mesma frase
com tools/tts_benchmark.py, mesmos ajustes e video. Depois executar
`python tools/tts_build_diagnostics.py` **na pasta onde este novo build foi feito**
e enviar seu JSON. Agora esperamos GLOBAL nas tres funcoes e kernel presente.
Se o build/checagem falhar, nao gravar manualmente para contornar a verificacao.

Verificacoes host de archive/link e politica do guard passaram, bem como as
regressoes existentes. Nao houve build ESP-IDF/CMake nem teste fisico aqui;
o ambiente nao possui essa toolchain nem CMake. O uso do target EXECUTABLE
apos project() foi conferido no fonte oficial ESP-IDF v6.1.1.
