# Contribuindo com o JrBot

Obrigado pelo interesse no JrBot. O projeto combina firmware embarcado, hardware, áudio, visão, painel local e, futuramente, serviços de IA. Para manter a evolução compreensível, toda contribuição deve deixar claro o que foi apenas implementado, o que compilou e o que foi validado fisicamente.

## Antes de começar

Leia:

1. [`docs/README.md`](docs/README.md)
2. [`docs/PROJECT_STATUS.md`](docs/PROJECT_STATUS.md)
3. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
4. [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md)
5. [`docs/TESTING.md`](docs/TESTING.md)

## Estratégia de branches

O projeto usa três níveis principais:

- `main`: baseline estável/promovido. Não desenvolver diretamente aqui.
- `develop`: integração da próxima versão.
- `feature/*` e `fix/*`: mudanças isoladas criadas a partir de `develop`.

Fluxo padrão:

```text
develop
   ↓
feature/minha-funcao
   ↓
Pull Request para develop
   ↓
build + testes + documentação
   ↓
develop
   ↓
Pull Request de promoção
   ↓
main
```

Antes de começar uma feature:

```bash
git switch develop
git pull --ff-only origin develop
git switch -c feature/nome-da-feature
```

Não misture várias capacidades independentes na mesma branch quando puder separá-las.

## Pull Requests

Todo PR deve informar:

- objetivo da mudança;
- arquivos/módulos afetados;
- impacto em hardware ou recursos compartilhados;
- estado de build;
- testes executados;
- validação física realizada ou ainda pendente;
- documentação atualizada;
- limitações conhecidas.

Uma feature experimental deve entrar primeiro em `develop`. A promoção para `main` só ocorre quando o baseline estiver suficientemente validado para ser tratado como referência estável.

## Hardware

Toda alteração de pinagem deve atualizar `hardware/pinmap.json` e os documentos de hardware relacionados.

Antes de enviar alterações de hardware, execute:

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

GPIOs compartilhados, alimentação e conflitos de periféricos precisam ser documentados explicitamente.

## Evidência de teste

Use os níveis definidos em [`docs/TESTING.md`](docs/TESTING.md):

- planejado;
- implementado;
- build verificado;
- teste de bancada/simulado;
- validado fisicamente.

Exemplos importantes:

- `ESP_OK` em transmissão I2S não prova que o som foi audível.
- JPEG recebido não prova foco ou qualidade óptica.
- evento simulado não prova reconhecimento acústico.
- log de reprodução não substitui confirmação humana de que a resposta foi ouvida.

## API e Flows

A Runtime API, o Playground e o Flow Engine possuem documentação de arquitetura antes da implementação completa. Não documente endpoints ou funções planejadas como disponíveis no firmware atual.

Ao adicionar uma nova capacidade, documente:

- nome público;
- argumentos;
- retorno;
- erros;
- segurança;
- persistência;
- eventos gerados;
- estado de implementação.

## Segurança

- Não faça a IA controlar GPIO diretamente.
- Não aceite upload ou comando arbitrário sem validação.
- Não exponha o portal local atual diretamente à Internet; ele não foi projetado como serviço público autenticado/TLS.
- Não adicione senhas, tokens, credenciais, áudio pessoal ou imagens privadas ao repositório.

## Licença

O repositório ainda não possui `LICENSE`. Não escolha ou adicione uma licença em nome do mantenedor sem decisão explícita do proprietário do projeto.
