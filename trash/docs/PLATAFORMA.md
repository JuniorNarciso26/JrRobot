# JrBot - arquitetura e IA

JrBot e um robo com IA em desenvolvimento. O firmware ativo esta na branch `v2-revisada`; a base de implementacao e ESP-IDF 5.5.x para ESP32-S3, nao um submodulo de outro firmware.

## Implementado

O painel Python/JavaScript local abre Serial USB (COM4 nesta montagem) ou comandos HTTP na rede local. O firmware valida comandos, responde com protocolo 2 e executa testes permitidos. O painel informa resultado e exporta log. Uma falha/desativacao do OLED nao interrompe o painel.

Modulos: rosto SSD1306 opcional; configuracao Wi-Fi; amplificador I2S sob demanda; camera de diagnostico de um quadro. A pinagem fixa vem de `hardware/pinmap.json`; audio exige tres pinos locais validos e aprovacao HW03. Nenhum teste inicia no boot.

## Planejado, nao entregue

Entrada de microfone, reconhecimento de fala, modelo de linguagem, sintese de voz, memoria de conversa, deteccao visual e movimentos. Ainda nao existe provedor/API/servidor de IA conectado nem chave necessaria para usar o diagnostico. IA nao pode contornar limites de GPIO, corrente ou movimento do firmware.

Uma arquitetura futura pode separar percepcao, processamento de IA e comandos locais validados. Local ou remoto, latencia, custos e privacidade precisam ser definidos antes da implementacao. Audio e imagem so devem sair da placa/computador com consentimento e finalidade claros.

## Restricoes atuais

Sem microfone configurado, sem controle de servo, sem streaming de camera e sem suporte SH1106 declarado. O portal HTTP nao possui autenticacao/TLS; nao expor a Internet. Painel deve permanecer em loopback. Nao apresentar recurso planejado como implementado.
