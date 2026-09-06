# JrRobot - aplicar a branch v2-revisada no ESP32-S3

Esta branch e destinada a validacao da V2. O OLED permanece em SDA=GPIO1 e SCL=GPIO2.

## Particularidade desta placa: duas USB / duas COM

Nesta placa do JrRobot ha duas conexoes USB com funcoes separadas:

- **COM6 = gravacao/flash do ESP32-S3**
- **COM4 = terminal serial / leitura de logs / envio de comandos**

Por isso, **nao usar `idf.py -p COM6 flash monitor` como fluxo principal**. Grave em COM6 e abra o monitor em COM4 separadamente.

## Windows - ESP-IDF 5.5.x

1. Abra o terminal ESP-IDF 5.5.x.
2. Entre no projeto:
   `cd C:\Projetos\JrRobotV2`
3. Atualize a branch:
   `git switch v2-revisada`
   `git pull origin v2-revisada`
4. Entre no firmware:
   `cd firmware`
5. Compile:
   `idf.py build`
6. Grave pela porta de programacao:
   `idf.py -p COM6 flash`
7. Depois abra o terminal pela outra USB:
   `idf.py -p COM4 monitor`
8. Para sair do monitor: `Ctrl+]`.

## Atualizacoes futuras desta branch

Dentro da pasta do projeto:

`git switch v2-revisada`
`git pull origin v2-revisada`
`cd firmware`
`idf.py build`
`idf.py -p COM6 flash`
`idf.py -p COM4 monitor`

## Terminal de comandos

O firmware usa UART0 para o terminal de comandos. Nesta montagem, use a USB correspondente a **COM4** para ler logs e enviar comandos como:

`status`

`help`

## Antes de gravar

- Placa alvo: ESP32-S3.
- OLED V2: SDA GPIO1, SCL GPIO2.
- Perfil inicial: face_only.
- Audio e camera permanecem desabilitados nesta fase de validacao.
- Nao alterar eFuses para estes testes.

## Se a gravacao falhar na COM6

Mantenha BOOT pressionado, pressione e solte RESET/EN, solte BOOT e execute novamente:

`idf.py -p COM6 flash`
