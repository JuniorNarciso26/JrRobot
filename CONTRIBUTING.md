# Contribuir com o JrBot

JrBot e um robo com IA em desenvolvimento. Trabalhe na `v2-revisada` ou em uma branch derivada; nao publique firmware experimental em `main` sem revisao.

Toda alteracao de hardware deve atualizar `hardware/pinmap.json`, executar `python tools/generate_pinmap.py` e atualizar materiais/esquema. GPIO21, GPIO41, GPIO42 e GPIO47 sao ocupados e nao podem ser reutilizados. A lista de candidatos do firmware nao substitui o esquema da placa.

Antes de enviar: rode `python tools/generate_pinmap.py --check` e `python tests/hardware/test_pin_policy.py`. Separe testes no computador, build ESP-IDF e teste na placa. Nao apresente resposta positiva de I2S como prova de som audivel, nem quadro JPEG como prova de foco.

Use o painel para operacao e logs. Para cada mudanca, informe versao do firmware, modelo exato da placa, pinagem, alimentacao e resultados. Nao adicione senhas, tokens, credenciais locais ou imagens/audio pessoais. Desligue todas as fontes antes de alterar fios e respeite as alimentacoes dos modulos.

A IA ainda esta no roteiro de desenvolvimento. Nao documente integracoes, APIs ou drivers planejados como se ja existissem. Preserve atribuicoes/licencas de terceiros e nao escolha uma licenca para o projeto sem aprovacao do proprietario.
