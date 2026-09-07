# Playground JrBot

**Status: PLANEJADO / design aprovado para desenvolvimento.**

O Playground será o ambiente de calibração e construção de comportamento do JrBot. Ele não treina uma rede neural no ESP32. Ele configura frases, mede o reconhecedor disponível e cria regras/Flows em runtime.

## Etapa 1 — cadastrar a palavra ou frase

O usuário informa uma frase, por exemplo:

```text
JR BOT
```

A frase é carregada temporariamente no mecanismo de reconhecimento para uma sessão de calibração.

Nenhuma ação do robô deve ser executada durante essa etapa.

## Etapa 2 — iniciar sessão contínua de teste

O usuário escolhe o intervalo mínimo entre resultados registrados, por exemplo:

```text
2 segundos
```

Depois pressiona **Iniciar teste**.

### Regra importante

O microfone continua ouvindo continuamente. O intervalo não significa “ligar o microfone a cada 2 segundos”; ele controla apenas a cadência mínima com que novas medições são aceitas/registradas.

Assim o usuário pode testar quantas vezes quiser e variar:

- velocidade da fala;
- distância;
- volume;
- entonação;
- posição em relação ao microfone;
- ruído do ambiente.

A sessão só termina quando o usuário clicar **Parar teste**.

## Etapa 3 — analisar e salvar confiança

O painel mostra cada amostra:

| # | Reconhecido | Confiança |
|---:|---|---:|
| 1 | JR BOT | 0.751 |
| 2 | JR BOT | 0.624 |
| 3 | JR BOT | 0.583 |
| 4 | JR BOT | 0.702 |

Também calcula, quando fizer sentido:

- quantidade;
- mínima;
- máxima;
- média.

O sistema pode sugerir um valor no futuro, mas o usuário escolhe a confiança mínima da regra.

```text
Confiança mínima: 0.58
```

O botão **Salvar confiança** persiste a configuração da frase/regra depois da validação.

## Etapa 4 — montar o comportamento

Depois da calibração, o usuário associa ações:

```text
voice("JR BOT", confidence=0.58)
-> face("happy")
-> say("oi")
-> listen()
```

O Playground deve oferecer inicialmente uma interface visual e, no futuro, um modo avançado para editar a representação completa do Flow.

## Modo calibração

Enquanto o Playground estiver testando uma frase:

- não executar `say`;
- não mudar rosto por detecção;
- não mover atuadores;
- não disparar Flows normais pela frase em calibração;
- publicar somente resultados e diagnóstico.

Isso evita que um teste com dezenas de repetições faça o robô responder dezenas de vezes.

## Estado conceitual da sessão

```text
IDLE
  -> STARTING
  -> CALIBRATING
  -> STOPPING
  -> REVIEW
  -> SAVED ou DISCARDED
```

## Dados mínimos por sessão

```json
{
  "session": "pg_001",
  "phrase": "JR BOT",
  "interval_ms": 2000,
  "samples": []
}
```

## Persistência

O botão de teste deve trabalhar em RAM primeiro. Somente uma ação explícita de salvar deve alterar configuração persistente.

Uma escrita incompleta ou configuração inválida não pode comprometer o boot do JrBot.

## Próximas fases

Depois da primeira versão:

- múltiplas frases por intenção;
- comparação entre perfis;
- export/import de configuração;
- histórico de calibração;
- assets WAV associados a `say`/`play`;
- editor visual de Flow;
- integração com `capabilities()`.
