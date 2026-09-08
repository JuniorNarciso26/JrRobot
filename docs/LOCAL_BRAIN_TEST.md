# JrBot - Local Brain / modo autonomo experimental

Branch: `feature/local-brain-autonomous`

## Objetivo desta etapa

Validar a arquitetura do primeiro cerebro local sem substituir o firmware V2 atual.

O painel ganhou um bloco **Modo autonomo experimental**. Pela conexao Serial USB, ele envia comandos especificos para o modulo `module_brain`.

Fluxo de bancada implementado:

1. clicar **Ativar autonomo**;
2. firmware entra em estado `listening`;
3. clicar **Simular chamado "JrBot"**;
4. firmware registra `wakeword_detected`;
5. OLED muda para a expressao `feliz`;
6. firmware registra `reaction_happy`;
7. volta ao estado de espera.

## Eventos esperados no log

```text
JR_BRAIN event=autonomous_on engine=bench_stub wakeword=JrBot
JR_BRAIN event=listening wakeword=JrBot note=custom_wakenet_pending
JR_BRAIN event=wakeword_detected source=bench keyword=JrBot confidence=1.000 trigger=1
JR_BRAIN event=reaction_happy expression=feliz
JR_BRAIN event=listening wakeword=JrBot
```

Ao desligar:

```text
JR_BRAIN event=autonomous_off
```

## Comandos de bancada

- `autonomo_on`
- `autonomo_off`
- `brain_status`
- `brain_test`

Nesta branch esses comandos sao interceptados no terminal USB utilizado pelo painel, preservando o protocolo V2 atual.

## Importante: reconhecimento falado real

O gatilho de bancada ainda **nao reconhece acusticamente a palavra "JrBot"**. Ele valida toda a cadeia posterior ao reconhecimento.

O proximo passo e substituir o `bench_stub` por um engine ESP-SR/WakeNet com modelo de palavra de ativacao personalizado. A interface `jr_brain_*` foi criada justamente para permitir essa troca sem reescrever o painel, o controle de estado ou as reacoes do robo.

Nao deve ser usado um detector generico de volume como se fosse reconhecimento de "JrBot", pois isso geraria falsos positivos e esconderia problemas do modelo de voz.
