# Teste A: diagnóstico isolado do alto-falante

Na branch `conserto-som`, o comando existente `audio_test` (também acessível
pelos botões **Testar audio** do painel local e **Testar som** do portal) chama
`jr_audio_test_tone(440, 2000)`. O tom de 440 Hz dura 2 segundos e usa o volume
já configurado. Não altera a transmissão WebRTC.

## Como verificar no JrBot HW04

1. Pare o Live e deixe o robô em modo idle. Ajuste o volume para um nível
   moderado, por exemplo `audio_volume 35`.
2. Envie `audio_test` pelo console serial ou pressione o botão de teste no
   painel existente. Não é necessário iniciar uma sessão Live.
3. Ouça se o alto-falante reproduz um tom contínuo e limpo. No monitor serial,
   confira `TEST_BEGIN` com `freq=440 dur=2000` e `TEST_END` com
   `result=ESP_OK tone_bytes=128000 i2s_released=1`.
4. Confirme `JR_OK audio_test=tx_completed ... freq=440 duration_ms=2000` e
   consulte `audio_diag` ou `status` para ver o último teste e `i2s_owner=none`.
5. Com o Live ativo, repita o comando: ele deve responder
   `JR_ERROR audio_test=live_active`, sem emitir o tom.

`ESP_OK` confirma a transmissão I2S, não a audibilidade. Registre o som ouvido
e o log serial para concluir a validação física do MAX98357A e do alto-falante.
O teste libera o canal e o barramento I2S ao terminar, inclusive em caso de
erro ou se o Live começar durante a execução.
