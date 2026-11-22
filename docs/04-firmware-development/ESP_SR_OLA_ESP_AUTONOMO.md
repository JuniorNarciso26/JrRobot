# Validação do WakeNet10 português no firmware principal do JrBot

**Branch experimental:** `feature/esp-sr-ola-esp-autonomo` (base `main`).

## Objetivo e escopo

Reutilizar a integração oficial ESP-SR 2.6.2 no firmware completo: Wi-Fi,
portal/painel, câmera, OLED, áudio e modo Live continuam presentes.

A função do modo Autônomo nesta etapa é somente:

1. No painel JrBot, ativar `Autônomo` (comando existente `autonomo_on`).
2. Abrir microfone MS3625 HW04 e selecionar `wn10_pt_olaesp`.
3. Ao ouvir **"Olá ESP"**, registrar `WAKEWORD_DETECTED` e disparar reação OLED existente.
4. Soltar o I2S do microfone, emitir um bip (880 Hz / 180 ms) no MAX98357A.
5. Liberar o alto-falante, aguardar 350 ms, reabrir o microfone e escutar outra vez.
6. Desativar `Autônomo` pelo painel com `autonomo_off` para liberar o barramento
   para Live e demais operações.

**Não há TTS, comando personalizado "Olá JrBot", áudio SAPI, assinatura pessoal,
MultiNet inglês, nova API nem alteração no painel nesta etapa.**
O teste independente `test/esp-sr-original` continua preservado.

## Dependências e recursos

- Espressif ESP-SR **2.6.2** via gerenciador do ESP-IDF.
- `CONFIG_SR_WN_WN10_PT_OLAESP=y`; `CONFIG_SR_MN_EN_NONE=y`.
- Modelo inserido na partição `model` já existente.
- Gravação do firmware feita pelo instalador principal; configurações de build
  isoladas como `sdkconfig.ola-esp-autonomo` e `b-ola-esp`.
- Não alterar pinos nem drivers do firmware principal.
- Como modelos e RAM são diferentes, medir uso de heap interna/PSRAM no HW04.

## Como testar no hardware

1. Rodar `INSTALAR.bat`; selecionar `feature/esp-sr-ola-esp-autonomo`.
2. Aguardar build/flash; abrir o painel como antes.
3. No painel, ligar o modo **Autônomo**; aguardar indicador de escuta.
4. Dizer **"Olá ESP"** três vezes, com intervalos de alguns segundos.
5. Cada detecção deve gerar exatamente um bip curto e retomar escuta.
6. Desligar Autônomo; testar Live e painel novamente para verificar regressões.

Os logs devem conter:

```text
JR_BRAIN event=autonomous_on engine=esp-sr-wakenet model=wn10_pt_olaesp wakeword=Ola ESP
WAKEWORD_DETECTED keyword=Ola_ESP model=wn10_pt_olaesp
WAKE_REPLY_BEGIN type=beep frequency=880Hz duration_ms=180
WAKE_REPLY_END result=ESP_OK
WAKE_LISTENING_RESUMED type=beep result=ESP_OK listening_resumed=1
```

Não confundir `TEST_END` do áudio ou `WAKE_REPLY_END` com comprovação
de som audível: confirmar pelo alto-falante.

## Critérios para incorporar ao main

- Firmware completo compila e grava com a partição do modelo, sem overflow.
- Painel acessível via Wi-Fi e modo Autônomo realmente inicia a escuta.
- Três acionamentos por "Olá ESP" com bip e retomada.
- Sem reinícios ou disputa I2S ao desligar Autônomo e entrar em Live.
- Revisar memoria livre, taxa de falsos acionamentos e logs físicos.

**Status atual: preparado em GitHub para validação. Não compilado nem
testado no HW04 nesta branch. Não fazer merge em `main` antes do teste.**

## Controle pelo celular (JrBotLocal)

Na mesma rede Wi-Fi do JrBot, abrir `http://jrbot.local/` (ou o IP do robô)
no navegador do celular. A página embarcada do próprio ESP32 mostra a nova
seção **Modo autônomo**, com os controles **Ligar autônomo** e
**Desligar autônomo**.

Esses botões usam a API existente `POST /cmd` e seus comandos
`autonomo_on` e `autonomo_off`, iguais aos comandos usados pelo painel.
O estado é consultado através de `brain_status` ao abrir a página
e a cada cinco segundos, evitando um botão fora de sincronia quando o
modo foi alterado pelo painel do computador.

**Não há serviço novo nem login:** a página é acessível somente pela rede
local, como os demais controles `/cmd`. Use apenas em rede confiável.
Durante o modo autônomo, o microfone reserva o I2S; desligar antes dos
testes de som e do Live.

### Teste do celular

1. Abrir `http://jrbot.local/` no celular, na mesma rede Wi-Fi.
2. Tocar em **Ligar autônomo** e verificar **Ligado e escutando Olá ESP**.
3. Falar "Olá ESP" e confirmar o bip (880 Hz) no MAX98357A.
4. Tocar em **Desligar autônomo** e confirmar estado **Desligado**.
5. Ligar pelo painel do computador e observar se o estado no celular muda.
6. Desligar pelo celular e confirmar no painel do computador.
7. Executar `node tests/hardware/test_jrbot_local_autonomous.cjs`
   para a verificação estática do HTML e do JavaScript embarcados.

**Status: publicado no branch experimental; a interface móvel ainda requer
teste em navegador real e no hardware.**
