# Módulo install

Este módulo representa a parte de instalação/gravação do JrBot.

Arquivos principais no pacote Windows:

- `INSTALAR.bat`: compila e grava no ESP32-S3.
- `CONFIGURAR_WIFI.bat`: gera `firmware/main/wifi_config.local.h` a partir de `credencial/wifi.txt`.
- `PAINEL.bat`: abre o painel local no computador.

Regra do projeto modular: se a instalação mudar, enviar apenas o módulo/install ou os `.bat` relacionados.
## Contrato do módulo

- O instalador oficial continua sendo `INSTALAR.bat` na raiz do pacote, para o Junior dar dois cliques/rodar no Windows.
- Esta pasta existe para documentar e versionar a parte de instalação separada do firmware.
- Se no futuro criarmos instalador novo, mexemos aqui e nos `.bat`, sem alterar `modules/face` quando o rosto já estiver aprovado.
