# JR_TTS_LONG_01 — referencia isolada e comparacao integrada

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
