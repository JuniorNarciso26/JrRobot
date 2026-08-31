# Contrato de roteamento JB

## Objetivo

O servidor deve executar regras antes de uma LLM. O firmware não envia uma intenção conhecida à LLM e não contém credenciais de provedor.

## Endpoint proposto

`POST /v1/jb/route`

```json
{
  "device_id": "opaque-id",
  "locale": "pt-BR",
  "text": "bom dia jb",
  "personality": "default",
  "context": {"hour": 8, "online": true}
}
```

`device_id` deve ser um identificador opaco; não inclua áudio bruto, nome do usuário ou credenciais em logs.

## Resposta de regra conhecida

```json
{
  "route": "RULE_SERVER",
  "intent": "GOOD_MORNING",
  "response_text": "Bom dia! Preparado para hoje?",
  "emotion": "happy",
  "allow_llm": false
}
```

## Resposta desconhecida

```json
{
  "route": "LLM_FALLBACK",
  "intent": "UNKNOWN",
  "allow_llm": true
}
```

## Garantias

- Evento local ou comando off-line conhecido: `LOCAL`, `allow_llm=false`.
- Texto STT conhecido: `RULE_SERVER`, `allow_llm=false`.
- `UNKNOWN` on-line: primeiro `RULE_SERVER`; somente a resposta desconhecida das regras habilita `LLM_FALLBACK`.
- `UNKNOWN` sem rede: `LOCAL`, `allow_llm=false`, com resposta curta de limitação local.

Este documento é um contrato de integração. Nenhum endpoint ou cliente de LLM foi implementado nesta etapa.
