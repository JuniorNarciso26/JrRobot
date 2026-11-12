# JrBot Voice Playground — exemplo oficial sem modificacoes

## Objetivo
Preservar **exatamente** o projeto original Espressif ESP-Skainet no GitHub, em um submodulo Git. Nao ha codigo C modificado ou escrito pelo JrBot aqui. Nao e uma versao de firmware JrBot testada fisicamente.

- Fonte oficial: https://github.com/espressif/esp-skainet
- Snapshot fixo: `97885d425d951b41a50747e2c33743d0e233a3e3` (10/10/2026).
- Submodulo local: `experiments/voice-playground/esp-skainet`.
- Exemplo para microfone ao vivo: `examples/wake_word_detection/afe`.
- Exemplo de reconhecimento com audio PCM pregravado (SEM microfone): `examples/wake_word_detection/wakenet`.
- Firmware JrBot e branch `main`: **nao alterados**.

## Baixar
No repositorio JrRobot:
```bash
git fetch origin
git switch feature/jrbot-v2-voice
git pull --ff-only origin feature/jrbot-v2-voice
git submodule update --init --recursive experiments/voice-playground/esp-skainet
```

## Teste da Espressif em placa de referencia suportada
O exemplo `afe` usa captura I2S **em tempo real** e depende do BSP `hardware_driver` da propria Espressif (configuracao de placa ESP32-S3-Korvo-1 indicada no README original). O arquivo `sdkconfig.defaults.esp32s3` escolhe o modelo **Hi Lexin** por padrao; e necessario selecionar **Hi ESP** em `menuconfig`, sem alterar nenhum arquivo-fonte:

```bash
cd experiments/voice-playground/esp-skainet/examples/wake_word_detection/afe
idf.py set-target esp32s3
idf.py menuconfig
# Audio Media HAL -> Audio hardware board -> ESP32-S3-Korvo-1
# ESP Speech Recognition -> Select wake words -> Hi ESP
idf.py build
# Em uma placa Korvo-1 suportada, apos verificar a porta:
# idf.py -p PORTA flash monitor
```

Quando detecta a palavra correta, o exemplo escreve `wakeword detected` no monitor Serial. Nao gera resposta falada pelo alto-falante.

## Aviso fundamental sobre o JrBot fisico
**Nao fazer flash do exemplo AFE inalterado na placa JrBot esperando que escute o microfone.** O driver BSP original configura os GPIOs e o hardware de audio da Korvo-1, que nao sao os mesmos do JrBot com microfone I2S e MAX98357A. Sem adaptacao de pinagem/driver, nao temos captura de audio do JrBot validada. Modificar `menuconfig` so seleciona os perfis de placa disponiveis na Espressif; nao cria um perfil JrBot. A adaptacao sera uma etapa separada, somente apos a validacao desta fonte intacta.

O exemplo `wakenet` tambem **nao testa o microfone**; ele processa buffers de audio de teste incorporados (`hiesp.h` / `hilexin.h`).

## Firmware e reversao
O projeto nao substitui arquivos da arvore `firmware/`. Flashar qualquer exemplo ESP-IDF no **mesmo** ESP32-S3 substitui temporariamente o firmware gravado, e pode alterar particionamento/configuracoes. Antes de gravar qualquer firmware de teste, salvar configuracoes e garantir o procedimento de reinstalacao da V1.8.3S. Nenhum flash foi feito nesta etapa.

## Estado e proximo passo
- [x] Fonte oficial referenciada integralmente e fixada por commit, sem modificacoes locais.
- [ ] Submodulo baixado na maquina do usuario.
- [ ] Compilacao com ESP-IDF 5.5.5 e versoes de dependencias registradas.
- [ ] Teste do exemplo no hardware de referencia da Espressif.
- [ ] Adaptacao opcional em projeto separado para I2S JrBot.
- [ ] Teste fisico de ativacao `Hi ESP` pelo microfone JrBot.
