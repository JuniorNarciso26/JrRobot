# Aplicar a branch v2-revisada nesta montagem

**Entrega atual: JRBOT-V2-DIAG-01, diagnostico SEM OLED.**

Leia [DIAGNOSTICO_SEM_OLED.md](DIAGNOSTICO_SEM_OLED.md) antes de gravar.
O roteiro `DIAG_V2.bat` usa build/configuracao separados dos arquivos antigos.

No terminal ESP-IDF 5.5.x, na raiz da copia atualizada:

```bat
DIAG_V2.bat build
```

Somente depois de compilacao bem-sucedida:

```bat
DIAG_V2.bat flash
DIAG_V2.bat monitor
```

COM6 grava; COM4 le logs e comandos nesta montagem. Nao use `flash monitor`
com uma unica porta. O Windows pode renumerar as COM; confira antes de gravar.

Procure `JRBOT-V2-DIAG-01` no boot e envie `version` e `status` pela COM4.
OLED nao e inicializado no modo padrao. GPIO1/2 continuam reservados para ele.
Audio e camera sao testes opt-in apos confirmar a montagem; nao iniciam no boot.
Microfone ainda depende de modelo/pinagem e nao e tratado como testado.
Nao altere eFuses, nao apague NVS e nao use reset Git destrutivo neste roteiro.

Testes desta entrega: fluxo em computador com APIs simuladas. Build ESP-IDF e
ensaio eletrico precisam ser feitos na sua instalacao e placa.
