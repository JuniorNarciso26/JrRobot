# Testes de software do JB

Estes testes não exigem uma placa ESP32-S3 nem GPIO configurado.

## Testes host-side

Os testes unitários em `jb-firmware/host_tests/` exercitam o roteador pt-BR,
a seleção de respostas, a política de rota e a proteção diária de eventos.

O `DailyEventGate` evita repetir um mesmo evento contextual no mesmo dia. A
persistência após reinicialização será integrada depois ao armazenamento seguro
da base Xiaozhi; ela não é simulada como existente nesta fase.

## Simulador

O arquivo `jb-firmware/host_tests/jb_simulator.cc` é uma ferramenta de PC que
chama as mesmas classes do firmware. Exemplo:

```text
> bom dia, JB
intent=GOOD_MORNING route=RULE_SERVER allow_llm=false reason=known_server_stt
> qual e a capital do Brasil?
intent=UNKNOWN route=RULE_SERVER allow_llm=false reason=server_rules_required
```

O primeiro resultado é `RULE_SERVER` porque o texto STT chega ao firmware
depois do envio do áudio; o contrato `POST /v1/jb/route` deve devolver uma
resposta conhecida e bloquear a LLM. Se as regras do servidor também retornarem
`UNKNOWN`, `DecideAfterRuleServer` permite `LLM_FALLBACK` somente com rede.

O primeiro diálogo de demonstração é `oi, JR`: após o STT, ele é classificado
como `GREETING`, seleciona uma resposta local (incluindo `Oi!`) e registra
`route=RULE_SERVER` com LLM bloqueada. A tela mostra a resposta; a saída de
áudio continua deliberadamente desativada até o hardware I2S ser definido.

Nenhum teste transmite áudio, conecta a Wi-Fi ou acessa uma LLM.

## Pacote de respostas na Flash

O build padrão inclui `manifest.json` e `intents.json` do pacote
`assets/packs/default/pt-BR/` na imagem de assets. O `ResponsePackLoader`
valida os dois documentos antes de criar um `ResponsePack`: versão 1, locale,
personalidade, nomes de intenção conhecidos, no mínimo três respostas por
intenção e tipos válidos para texto, áudio, emoção, peso e condições.

Nesta etapa o carregador está pronto para receber os bytes da partição de
assets. A aplicação o chama depois de `assets.Apply()` e expõe
`Application::HandleJbLocalEvent()` para os futuros eventos de relógio,
presença e câmera. A primeira saída é intencionalmente só um log com
`route=LOCAL`; a reprodução local só será conectada depois da definição da
placa JB, pois não há saída de áudio pt-BR confirmada.
