# Planejamento e evolução do JrBot

Este documento apresenta **o que foi concluído, o que está em integração e o que vem depois**. O detalhamento de cada versão fica no [Histórico de versões](CHANGELOG.md).

## Evolução documentada

| Versão | Entrega principal | Situação |
| --- | --- | --- |
| **V1.6.1** | Controle via navegador, câmera, áudio e interfone PTT | Revisão histórica |
| **V1.6.2** (19/09/2026) | Instalador com seleção dinâmica de branches | Registrada |
| **V1.6.3** (23/09/2026) | Live WebRTC local: áudio bidirecional e vídeo JPEG | Validada fisicamente |
| **V1.7.04** (28/09/2026) | Gestão central de modos e recursos/I2S | **Última versão estável promovida à `main`** |
| **V1.8S** (versão definida) | Integração do JrBot com a JrSkill Network | **Integração preparada na `V1s-00`; promoção pendente** |

A versão **V1.8S** preserva as funções existentes do robô e adiciona o ecossistema de Skills. Durante o Colosseum 2026, o fluxo de criação, publicação, aquisição de licença e execução física foi validado em **Solana Devnet**. O firmware experimental desse marco ainda se identifica como `JrBot_V1S_APP_03`; **V1.8S é o nome de produto definido para a próxima promoção**, não uma release já publicada.

## Frentes de evolução

| Frente | Situação documentada | Próximo objetivo |
| --- | --- | --- |
| **JrBot V1.8S** | Funções V1 e integração JrSkill validadas em seus respectivos testes | Concluir testes do conjunto integrado, alinhar identificação e promover à `main` |
| **Plataforma JrSkill** | Creator Studio, Store, licenças e execução demonstrados na Devnet | Ampliar ferramentas de criação e publicação de Skills |
| **Montagem e corpo 3D** | Montagem eletrônica documentada; projeto mecânico registrado na [Issue #22](https://github.com/JuniorNarciso26/JrRobot/issues/22) | Reunir modelos imprimíveis, documentar corpo e validar a montagem completa |
| **JrBot V2** | Linha de desenvolvimento de voz local preservada | Evoluir reconhecimento e comandos locais |
| **JrBot V3** | Runtime API existente como base técnica | Definir futura linha de controle programático |
| **JrBrain** | Visão de longo prazo | Explorar memória, contexto, personalidade e integração de IA |

As frentes de corpo mecânico e JrSkill podem evoluir em paralelo ao firmware. Não há neste documento datas de lançamento confirmadas para V2, V3, JrBrain ou para uma plataforma Mainnet.

## Marcos necessários antes de promover uma versão

1. Identificar claramente firmware, branch e hardware de referência.
2. Compilar a versão que será promovida.
3. Executar os testes funcionais e físicos pertinentes.
4. Registrar resultados, limitações e pendências.
5. Atualizar documentação e [Histórico de versões](CHANGELOG.md) antes da promoção.

A distinção entre **compilado**, **testado em bancada** e **validado fisicamente** segue o [guia de testes](../04-firmware-desenvolvimento/TESTING.md).

## Onde consultar os detalhes

- [Estado técnico e branches](../04-firmware-desenvolvimento/PROJECT_STATUS.md)
- [Montagem e hardware](../02-montagem/README.md)
- [JrSkill Network](../05-jrskill-network/README.md)
- [Diário do Colosseum 2026](../06-historico-colosseum/HACKATHON_DEVLOG.md)
