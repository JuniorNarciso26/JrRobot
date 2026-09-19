# Changelog do JrBot

Este arquivo registra as mudanças promovidas para a linha de integração do produto.

## JrBot_V1.6.2 — 2026-09-19

Base: `JrBot_V1.6.1`.

### Melhorias

- `INSTALAR.bat` deixou de depender de uma lista fixa de branches;
- o menu passa a consultar todas as branches remotas ativas diretamente no GitHub;
- branches `feature/*`, `fix/*` e `hotfix/*` podem ser selecionadas para instalação e teste;
- branches `archive/*` permanecem ocultas;
- a branch atualmente usada é indicada com `[ATUAL]`;
- o mesmo fluxo continua responsável por sincronizar, compilar, gravar e abrir o painel.

### Compatibilidade

- nenhuma mudança de hardware;
- nenhuma mudança no PTT half-duplex;
- nenhuma mudança na arquitetura de áudio da V1;
- nenhuma mudança de contrato da Runtime API.

### Processo

A partir desta revisão, toda promoção de versão para `develop` deve atualizar também a documentação e este changelog.

## JrBot_V1.6.1

Revisão anterior da linha V1, com a experiência de navegador consolidada, câmera com orientação padrão de 90 graus, áudio/microfone, HTTPS local e interfone PTT half-duplex.
