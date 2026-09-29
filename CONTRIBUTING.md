# Contribuindo com o JrBot

Obrigado pelo interesse no JrBot.

O projeto combina firmware embarcado, hardware, áudio, visão, WebRTC, interfaces, inteligência artificial e uma linha paralela de pesquisa em Skills distribuíveis.

## Antes de começar

Leia:

1. [Documentação](docs/README.md)
2. [Estado do projeto](docs/PROJECT_STATUS.md)
3. [Roadmap](docs/ROADMAP.md)
4. [Arquitetura](docs/ARCHITECTURE.md)
5. [Desenvolvimento](docs/DEVELOPMENT.md)
6. [Testes](docs/TESTING.md)

## Branches oficiais

- `main` — release oficial estável;
- `develop` — integração;
- `V1s-00` — linha oficial paralela JrSkill Network / Solana;
- `v2` — voz local;
- `v1` — referência histórica da V1.

Não desenvolver diretamente em `main`.

### Contribuições para a baseline do produto

Parta de `develop` e use uma branch dedicada:

```text
feature/<nome>
fix/<nome>
hotfix/<nome>
test/<experimento>
```

### Contribuições para JrSkill Network

A linha oficial é `V1s-00`.

Ela estuda Skills portáveis e declarativas, uma camada de distribuição/licenciamento baseada em Solana e execução segura por capabilities locais.

O andamento, as etapas e a arquitetura experimental detalhada dessa linha devem ser documentados **na própria V1s-00**, não duplicados na documentação da `main`.

### Contribuições para voz local

Use a linha `v2` quando o trabalho for específico de reconhecimento/comandos por voz local.

## Processo

```text
Issue
→ pesquisa / arquitetura
→ branch
→ implementação
→ build
→ teste
→ validação física
→ documentação
→ Pull Request
→ decisão
```

Experimentos de laboratório não devem ser promovidos diretamente para `develop`.

## Pull Requests

Todo PR deve informar:

- objetivo;
- Issue relacionada;
- base/branch;
- arquivos e módulos afetados;
- impacto em hardware ou recursos compartilhados;
- status de compilação;
- testes executados;
- validação física realizada ou pendente;
- limitações;
- documentação atualizada.

## Hardware

Mudanças de pinagem devem atualizar `hardware/pinmap.json` e os documentos relacionados.

Antes de enviar alterações:

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

GPIOs compartilhados, alimentação e conflitos de periféricos precisam ser documentados explicitamente.

## Níveis de evidência

Use os níveis definidos em [docs/TESTING.md](docs/TESTING.md):

- planejado;
- implementado;
- build verificado;
- bancada/simulado;
- validado fisicamente.

Não confunda sucesso de software com validação física.

## Segurança

- IA/Skill não controla GPIO diretamente;
- funções externas precisam estar allowlisted;
- argumentos precisam ser validados;
- não aceite código arbitrário;
- não exponha o portal local diretamente à Internet;
- não publique credenciais ou dados pessoais.

## Licença

O JrBot é distribuído sob a [MIT License](LICENSE).

Contribuições aceitas são disponibilizadas sob a mesma licença, respeitadas licenças de terceiros.
