# JrBot V1.8.2S — Mesa Mix da Live

Versão atual em `main`: `JrBot_V1.8.2S`  
Origem: branch preservada `feature/live-audio-lab` e [PR #57](https://github.com/JuniorNarciso26/JrRobot/pull/57), inicialmente identificada como `JrBot_V1.8.1S_AUDIO_LAB_01`.

## Objetivo

A **Mesa Mix · Live** permite ajustar o áudio WebRTC no próprio JrBot, sem mudar o hardware nem acrescentar outra fila de áudio. Todos os controles são **temporários em RAM**; reiniciar a ESP32-S3 restaura os valores padrão. A implementação **não possui cancelamento acústico de eco (AEC) completo**.

Abra **https://jrbot.local/** (ou HTTPS pelo IP do JrBot), localize **Câmera / Ao vivo WebRTC** e expanda **Mesa Mix · Live**. O controle fica na aplicação web do robô, não no painel Windows `PAINEL.bat`.

## Controles

| Controle | Padrao | Intervalo | Comportamento |
| --- | ---: | ---: | --- |
| Volume do alto-falante WebRTC | 100% | 0-100%, passo 5 | Escala PCM apos decodificar PCMA, exclusivamente na Live |
| Ganho digital do microfone | 100% | 25-150%, passo 5 | Escala o PCM enviado por PCMA; valores acima de 100% podem aumentar saturacao |
| Mic enquanto a caixa toca (ducking) | 100% | 0-100%, passo 10 | Aplica atenuacao por cerca de 250 ms apos reproducao significativa; 100%=sem atenuacao; **nao e AEC** |
| Porta de ruido (noise gate) | 0 | 0-2000 PCM, passo 100 | Zera amostras de baixa amplitude; 0=desligada; nao remove chiado que esteja acima do limiar |
| Pre-buffer | 20 ms | 20/40/60 ms | Aguarda 1/2/3 pacotes na fila existente antes de iniciar/retomar; pode aumentar latencia |

A taxa permanece **PCMA 8 kHz mono** na rede e **I2S 16 kHz** no hardware. Mudar codec/taxa exigiria renegociar o WebRTC e fica fora desta primeira etapa. O volume do menu **Som** (reproducao sob demanda) continua independente do volume WebRTC.

Clique **Aplicar ajustes** para atualizar o firmware sem interromper a Live; clique **Restaurar padrao** para retornar aos valores neutros. **Baixar diagnostico** exporta JSON, com configuracao, falhas e memoria.

### Ajuste fino durante a conversa

1. Use fones de ouvido e inicie a Live com os valores neutros. Ajuste um controle por vez para conseguir comparar o resultado.
2. Reduza o **volume do alto-falante da Live** e o **ganho digital do microfone** gradualmente. O usuario relatou melhora perceptivel do som ao reduzir bastante ambos enquanto o microfone estava aberto; ainda nao ha percentuais medidos que sirvam como perfil universal.
3. Se houver retorno da voz enquanto a caixa reproduz, teste `echo_duck` abaixo de 100%. Em 100% nao ha atenuacao; em 0% o microfone fica totalmente atenuado nesse intervalo e a conversa perde simultaneidade. Este controle nao cancela o eco que ja entrou no sinal.
4. Mantenha o noise gate em 0 inicialmente. Aumente o limiar apenas se houver ruido entre falas e verifique se palavras baixas nao sao cortadas.
5. Mantenha o pre-buffer em 20 ms quando a prioridade for menor atraso. Compare 40/60 ms apenas se houver cortes ou falhas de buffer; aumentar esse valor pode acrescentar latencia.
6. Baixe um diagnostico com a configuracao escolhida e anote separadamente a qualidade percebida e o atraso de retorno da propria voz. Os controles voltam ao padrao depois de reiniciar o JrBot.

A API da Mesa Mix mantém o caminho histórico `/webrtc-audio/lab` para compatibilidade e é registrada somente na instância HTTPS:
- `GET /webrtc-audio/lab` — configuracao vigente e contadores;
- `POST /webrtc-audio/lab` — corpo JSON com os cinco campos obrigatorios `volume`, `mic_gain`, `echo_duck`, `gate`, `prebuffer`. As faixas sao validadas.

## Memoria e desempenho

- **Nao** ha buffers extras para amostras, audio gravado ou cancelamento de eco.
- O firmware reutiliza a fila preexistente de 12 pacotes (`JR_WA_AUDIO_QUEUE_DEPTH`), preferencialmente alocada na PSRAM.
- Cada iteracao de 20 ms consulta cinco inteiros de configuracao; a interface coleta metricas a cada 5 segundos **somente enquanto expandida**.
- Novos contadores: `queue_pending`, `queue_peak`, `underruns`, `mic_clip`, `mic_peak`, `speaker_peak`, `internal_free`, `internal_largest`, `internal_min`, `psram_free`.
- O indicador de underrun considera a ausencia de pacote apos a reproducao ter iniciado, com pacote recebido recentemente. Nao equivale a diagnostico definitivo de jitter de rede.
- A saturacao conta ocorrencias observadas na conversao do microfone e no ganho digital, sem identificar sozinho se o ruido e eletrico ou acustico.
- A UI e atendida como JavaScript estatico embarcado; seus controles rodam no navegador, **nao** como outra tarefa de audio no ESP32.

Leia os contadores junto com a experiencia de audio: `mic_clip` indica saturacao acumulada do microfone; `underruns` indica falta de pacote na fila; `queue_pending`/`queue_peak` mostram sua ocupacao; `internal_free`, `internal_largest` e `internal_min` mostram RAM interna atual, maior bloco e minima livre; `psram_free` mostra PSRAM livre. Compare leituras no inicio e depois de 5-10 minutos da mesma Live. Sem JSONs medidos antes/depois, nao e possivel afirmar que o consumo de memoria ficou estavel.

## Resultado relatado em 7 de novembro de 2026

- O usuário executou a versão de teste que originou a V1.8.2S e relatou que a comunicação funciona e soa bem com fones de ouvido.
- Reduzir bastante o ganho do microfone e o volume da caixa com o microfone aberto melhorou o audio percebido. Os valores exatos e as metricas RAM/PSRAM nao foram fornecidos.
- O retorno da própria voz ainda chega com atraso, inclusive com fones e longe do robô. A origem e a duração desse retorno ainda não foram medidas; a Mesa Mix não deve ser descrita como solução completa de eco ou latência.
- A observacao cobre o audio percebido pelo usuario. Nao substitui um registro de teste prolongado de video, I2S, RAM interna e PSRAM.

### Validacao de software em 7 de novembro de 2026

Uma cópia isolada do código da PR #57 compilou com **ESP-IDF 5.5.5**, alvo ESP32-S3 e `sdkconfig.develop_wifi_ram_light`. A versão final **V1.8.2S** também compilou com esse perfil: binário de aplicativo `0x25ed50` bytes, com 53% da menor partição livre. O pinmap HW04, os casos `final_default`/`stale_old_config` e a sintaxe do JavaScript embarcado passaram. Essa validação não gravou uma placa e não mede RAM/PSRAM durante a Live.

## Protocolo de teste fisico sugerido

1. Em uma instalacao de referencia da V1.8.1S, anotar memoria livre da Live, qualidade percebida, eco e chiado. Os registros anteriores do usuario indicaram cerca de 43 KiB de RAM interna livre durante a Live, mas a nova medicao precisa ser feita no mesmo ambiente para comparacao.
2. Gravar a V1.8.2S e **confirmar o identificador `JrBot_V1.8.2S` na página**. Manter o JrBot e o celular em ambientes separados.
3. Iniciar a Live com **todos os valores no padrao**. Repetir voz curta por 1 minuto e baixar diagnostico.
4. Volume da Live: testar **100%, 60%, 35%**; comparar se a voz fica menos distorcida e se o eco diminui.
5. Ganho do microfone: comparar **100%, 70%, 50%**; monitorar `mic_clip` e inteligibilidade no celular.
6. Atenuacao durante reproducao: comparar **100%, 50%, 0%**; perceber se o eco cessa e se fica impossivel interromper a fala (o `0%` impede full-duplex enquanto a caixa toca).
7. Pre-buffer: comparar **20, 40 e 60 ms**. Medir se a voz acelerada/picotada melhora e o quanto cresce o atraso.
8. Gate: comparar **0, 200 e 500**; testar se palavras baixas ficam cortadas. Voltar a 0 se piorar.
9. Usar a camera em **320x240** e depois **640x480**, com a mesma configuracao de audio.
10. Manter a Live por 5-10 min, verificar erros de I2S/WebRTC e estabilidade da RAM. Gravar os JSONs e os logs Live para comparar.

**Critério para avaliações futuras:** voz compreensível sem piora significativa da latência, nenhuma regressão no WebRTC/vídeo, sem crescimento progressivo do uso de memória, sem erros I2S e comparação objetiva de qualidade/eco. O teste de áudio relatado acima é positivo; a medição prolongada de memória e a investigação do retorno atrasado ainda não foram registradas.

## Limites e seguranca

- Nenhuma modificacao fisica ou eletrica na placa.
- Nao ha NVS/flash para salvar o perfil; o estado e somente da sessao de firmware, persistindo entre Lives mas resetado no reboot.
- Atenuação e noise gate são ajustes leves; não substituem AEC nem corrigem problemas de alimentação ou aterramento do MAX98357A.
- Controle remoto pela Internet e autenticacao da mesa ficam fora do escopo; usar apenas a rede local confiavel e HTTPS existente.
- A V1.8.2S foi promovida como versão final com a Mesa Mix e com as limitações acima registradas. Em futuras revisões de áudio, registrar a compilação, os testes físicos e quaisquer medições de memória e latência disponíveis.
