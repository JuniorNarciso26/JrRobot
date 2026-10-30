# Referência isolada para saída de áudio HW04

O [sketch Arduino](../../examples/audio_reference_hw04/audio_reference_hw04.ino)
usa somente `ESP_I2S`, GPIO21/47/42 e o MAX98357A. Ele não chama o código
JrBot nem usa microfone, Wi-Fi ou WebRTC. A reprodução de voz usa um arquivo
WAV gravado no computador; assim a qualidade do alto-falante pode ser avaliada
sem depender da captação do MS3625.

O exemplo enviado pelo usuário escreve uma onda **quadrada** de amplitude 500
com amostragem de 8 kHz, estéreo de 16 bits. O divisor inteiro gera cerca de
444 Hz. Esse tom pode ser baixo e não informa se uma voz será inteligível.
O sketch tem ainda a opção de amplitude 5000 e reproduz voz PCM16 mono a
16 kHz, duplicada nos canais esquerdo e direito do I2S.

## Preparação e compilação (sem gravar a placa)

1. Use Arduino-ESP32 3.x e a placa `ESP32S3 Dev Module` com `Flash Size=16MB`,
   `PSRAM=OPI`, `USB Mode=Hardware CDC and JTAG` e `USB CDC On Boot=Enabled`.
2. Compile `examples/audio_reference_hw04/audio_reference_hw04.ino`. Via CLI:

   ```powershell
   arduino-cli compile --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi' examples/audio_reference_hw04
   ```

3. Grave esse sketch **somente após autorização**. Ele substitui
   temporariamente o firmware JrBot na flash. Depois, o `INSTALAR.bat`
   restaura o firmware JrBot. Este procedimento não faz parte do instalador.

## Teste físico após a gravação autorizada

Feche o painel JrBot para liberar a porta COM. Instale `pyserial` no Python que
executará o script: `python -m pip install pyserial`. Escolha a porta COM do
sketch (ela pode mudar após a gravação).

```powershell
python tools/audio_reference_send.py COM6 --tone
python tools/audio_reference_send.py COM6 --tone-loud
python tools/audio_reference_send.py COM6 caminho\voz.wav
```

O arquivo deve ser WAV PCM 16-bit, mono, 16 kHz e ter até 10 segundos. Ouça-o
no computador antes de enviar, para assegurar que a gravação já é nítida.
`VOICE_DONE` confirma que as amostras foram enviadas ao I2S; o ouvido é a
verificação da qualidade. `clipped` indica quantas amostras do arquivo já
estavam no limite digital. Se o tom e a voz soarem ruins no sketch isolado,
investigue alimentação, aterramento, alto-falante, conexões I2S e ganho do
MAX98357A. Se soarem bons ali e ruins no JrBot, compare o formato e o ganho
dos caminhos de reprodução do firmware.

No log do Teste A, `audio_volume 93` gerou uma senoide com amplitude de cerca
de 24.180 em 32.767, muito acima dos 500 do exemplo. Antes de atribuir a
falha ao barramento I2S, repita o teste atual com volume moderado (`10` a
`20`). A resposta `ESP_OK` confirma transmissão, não som limpo.

O firmware JrBot também permite escolher um arquivo de voz no portal
`http://<ip-do-jrbot>/` → **Mensagem do celular** → **Escolher áudio** →
**Tocar no JrBot**. Essa rota converte o arquivo para WAV PCM16 mono 16 kHz.
Ela exige Wi-Fi configurado; o log de 30/10/2026 indicou que não havia rede
salva. A gravação pelo microfone do JrBot depende do MS3625, que apareceu
indisponível naquele log.
