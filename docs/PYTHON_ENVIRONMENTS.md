# Python do painel e ESP-IDF

O painel JrBot e o ESP-IDF possuem conjuntos de dependencias Python diferentes e devem usar ambientes separados.

## Regra

- ESP-IDF: usa o ambiente criado pelo instalador oficial da Espressif.
- Painel JrBot: usa `%LOCALAPPDATA%\JrBot\panel-venv`.
- `tools/jrbot_frontend/requirements.txt` nunca deve ser instalado dentro de `C:\Espressif\python_env\...`.

## Incidente de 2026-10-06

Durante a primeira tentativa de compilar `JrBot_V1S_APP_01`, o ESP-IDF 5.5.5 recusou iniciar porque seu ambiente continha `cryptography 50.0.1`, enquanto o constraint oficial exigia `cryptography<45,>=2.1.4`.

Causa identificada: `PAINEL.bat` utilizava o primeiro `python` do PATH. Quando aberto depois do `export.bat` do ESP-IDF, esse executavel era o Python privado do ESP-IDF. O script entao instalava as dependencias do painel naquele ambiente.

Correcao: `PAINEL.bat` agora cria e usa um venv dedicado fora do repositorio, atraves do launcher Windows `py -3`.

## Estado

A alteracao evita nova contaminacao do ambiente do ESP-IDF. Um ambiente do ESP-IDF que ja tenha sido alterado precisa ser reparado uma vez antes da proxima compilacao.
