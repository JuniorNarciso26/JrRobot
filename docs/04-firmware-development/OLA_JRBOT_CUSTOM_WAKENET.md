# Futuro WakeNet personalizado: "Olá JrBot"

**Base atual: JrBot V2.0, branch `main`.**
Esta página preserva o estudo feito em `feature/ola-jrbot-wakenet`
antes da limpeza dos branches experimentais.

## Decisão

O JrBot V2.0 já reconhece **"Olá ESP"** localmente, por meio do modelo
oficial ESP-SR 2.6.2 `wn10_pt_olaesp`. Este modelo foi validado no
ESP32-S3 HW04 junto ao firmware completo, ao bip de confirmação, à
expressão Love e ao controle autônomo no celular.

A próxima expressão desejada é **"Olá JrBot"**. A simples troca do nome
no código **não altera os pesos do WakeNet**, portanto não fará o
reconhecimento funcionar. Não existe modelo customizado treinado no
repositório, nem arquivo entregue para testar.

**Não modificar o modelo V2.0 agora.** Quando o arquivo do modelo oficial
estiver disponível, criar um branch **novo a partir da `main` atual**,
incorporá-lo ao ESP-SR e validar no HW04. Assim, a manutenção do
reconhecimento "Olá ESP" continua independente do próximo experimento.

## Aquisição e integração futura

1. Confirmar a pronúncia "Olá JrBot" em português brasileiro e as
   condições de licença, redistribuição e atualização do modelo
   personalizado com a Espressif.
2. Solicitar ou produzir um modelo compatível com ESP32-S3 N16R8,
   **ESP-IDF 5.5.x e ESP-SR 2.6.2**, informando formato, versão
   e tamanho de armazenamento.
3. Receber a pasta completa do modelo e metadados, incluindo
   `_MODEL_INFO_`. Um nome de pasta ou modelo fictício não basta.
4. Criar novo branch de teste a partir da `main`, nunca recriar
   os branches antigos automaticamente. Ajustar seleção/carga de modelo
   e partição apenas após conhecer o artefato real.
5. Testar a detecção por várias pessoas, falsos disparos, tempo de
   resposta, RAM interna/PSRAM e coexistência com Wi-Fi, Live, câmera,
   áudio e OLED. Preservar a opção de reverter para o V2.0.
6. Aprovar fisicamente antes de promover o branch ao `main`.

A documentação do estudo original registra a personalização comercial
do WakeNet e uma alternativa de protótipo TTS-only; confirmar condições
atuais diretamente com o fornecedor. O experimento anterior de aliases
MultiNet ingleses e a comparação de assinaturas acústicas (DTW) não
produziram um modelo confiável de "Olá JrBot" e **não** são a base da V2.0.

## Ferramenta preservada na main

`tools/wakenet_ola_jrbot.py` é uma ferramenta opcional de checagem,
sem dependências adicionais, **não integrada ao firmware**:

```bat
python tools\wakenet_ola_jrbot.py status
python tools\wakenet_ola_jrbot.py corpus
python tools\wakenet_ola_jrbot.py modelo
```

Ela identifica se existem WAVs mono PCM 16-bit / 16 kHz em
`experiments/ola-jrbot-wakenet/dataset` e uma pasta de modelo com
`_MODEL_INFO_` em `experiments/ola-jrbot-wakenet/models`.
O comando não treina modelos, não verifica integridade/compatibilidade
dos pesos e não faz flash. As pastas são opcionais e não são exigidas
para compilar ou usar a V2.0. **Não publicar gravações de voz privadas**.

Referências técnicas:
- https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/ESP_Wake_Words_Customization.html
- https://github.com/espressif/esp-sr/blob/master/wakeword_list.md

**Status:** plano e verificador preservados na `main`. Implementação
do modelo personalizado pendente de receber o arquivo real.
