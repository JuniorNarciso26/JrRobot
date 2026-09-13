# Fluxo atual de branches

O padrão oficial permanece o definido no README e no CONTRIBUTING:

```text
main
  <- promoção de develop

develop
  <- PRs de feature/* e fix/*

feature/* e fix/*
  <- criadas a partir de develop

archive/*
  <- snapshots históricos
```

A linha atual de voz e Playground fica consolidada em:

```text
feature/voice-playground-v1 -> develop
```

As candidatas `JRBotV2_RUNTIME_API_V1_03`, `_04` e `_05` são revisões sucessivas da mesma feature. Uma nova candidata de firmware não exige uma nova branch quando continua dentro do mesmo escopo funcional.
