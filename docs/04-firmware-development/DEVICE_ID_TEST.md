# JrBot — teste de leitura do Device ID

Objetivo: ler o MAC de fabrica do ESP32-S3 gravado em eFuse e gerar
JRBOT-XXXXXXXXXXXX. Este e o primeiro teste para a identidade proposta
na Issue #1 do JrBot_Cloud.

## No PC conectado ao JrBot por USB

O teste e somente leitura: nao apaga nem grava firmware/eFuse.
A conexao pelo bootloader pode reiniciar temporariamente o robo.
Feche monitor serial, Arduino Serial Monitor ou IDF Monitor antes de iniciar.

No terminal, na raiz do JrRobot:

~~~powershell
python -m pip install esptool
python tools/ler_mac_esp32.py --list-ports
python tools/ler_mac_esp32.py --port COM6
~~~

Substitua COM6 pela porta detectada. Em Linux, por exemplo: /dev/ttyACM0.
Se nao conectar, verifique o cabo USB de dados e o modo de download.
Quando necessario: segure BOOT, pressione RESET, solte RESET e depois BOOT.
Apos o teste reinicie o robo normalmente.

Saida esperada (exemplo ficticio):

~~~text
MAC_EFUSE=34:85:18:AB:CD:EF
DEVICE_ID=JRBOT-348518ABCDEF
RESULT=PASS
~~~

A leitura no hardware ainda precisa ser executada pelo responsavel com a placa.
O MAC e identificador publico, nao autenticacao. Um dispositivo precisa de
credencial criptografica individual e de vinculo autorizado com a conta.

## Validacao offline sem placa

~~~sh
python -m unittest discover -s tests/device_id -p "test_*.py" -v
~~~

Esses testes exercitam parser e comandos esptool v4/v5 simulados; nao sao
evidencia de leitura real do MAC.

Registre data, porta serial, resultado PASS ou FAIL e eventual mensagem de erro.
Nao publique dados de identificacao desnecessariamente.

Nenhuma parte desse teste configura MQTT, Cloud ou acesso remoto.
Nenhum arquivo do firmware estavel e modificado.

Referencia oficial:
https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html
