# Estado técnico atual do JrBot

Este documento concentra informações de firmware, hardware de referência e branches. O panorama de versões e objetivos está no [Planejamento e evolução](../01-historia-roadmap/ROADMAP.md).

## Versões e branches

| Referência | Estado |
| --- | --- |
| **`main`** | Última versão oficialmente promovida: `JrBot_V1.7.04` |
| **`V1s-00`** | Integração JrSkill para a versão de produto **V1.8S**; firmware atual `JrBot_V1S_APP_03` |
| **`develop`** | Linha de integração |
| **`v2`** | Desenvolvimento de voz local, preservado separadamente |
| **`v1`** | Referência histórica da V1 |

A `V1.7.04` foi validada fisicamente para câmera OV5640, áudio WebRTC bidirecional, JPEG/DataChannel e gestão central de modos e I2S. O fluxo JrSkill foi validado separadamente em **Solana Devnet**, com execução física da Skill `jrteste` em 07/10/2026. **Os testes da integração final para promoção à `main` permanecem pendentes.**

## Hardware de referência

**JRBOT-HW-04:** ESP32-S3 N16R8, câmera OV5640, OLED SSD1306, microfone MS3625 e amplificador MAX98357A.

O áudio compartilha BCLK (GPIO21) e WS (GPIO47); os sinais de dados utilizam GPIO41 (microfone) e GPIO42 (amplificador). Para montagem, alimentação e conexões completas, consulte [Revisão HW04](../02-montagem/REVISAO_HW04.md), [Pinagem](../02-montagem/PINAGEM.md) e [Esquema de ligação](../02-montagem/ESQUEMA_LIGACAO.md).

## Comportamento técnico e limitações

A V1.7.04 consolidou `jr_mode_manager`, `jr_resource_manager` e ownership do I2S para microfone, reprodução e Live. Limitações registradas: cancelamento de eco acústico (AEC), WebRTC pela Internet e eventos de backpressure de vídeo. O controle por voz local segue uma frente separada (V2).

Consulte [Arquitetura](ARCHITECTURE.md) e [Testes](TESTING.md) para os critérios técnicos.

## Instalação e identificação

O `INSTALAR.bat` consulta branches remotas, sincroniza a escolhida e permite compilar, gravar e abrir o painel. Confirme a **versão efetivamente instalada** no log/status; o nome da versão comercial não substitui o identificador da build.

## Documentação de uma nova promoção

Ao promover uma versão, revisar:

1. `firmware/version.txt`;
2. `README.md` principal;
3. [Planejamento e evolução](../01-historia-roadmap/ROADMAP.md);
4. [Histórico de versões](../01-historia-roadmap/CHANGELOG.md);
5. este estado técnico e os documentos dos módulos afetados.

A documentação de Skills fica em [JrSkill Network](../05-jrskill-network/README.md) e a cronologia do hackathon em [Colosseum 2026](../06-historico-colosseum/README.md).
