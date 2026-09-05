# JrRobot - aplicar a branch v2-revisada no ESP32-S3

Esta branch e destinada a validacao da V2. O OLED permanece em SDA=GPIO1 e SCL=GPIO2.

## Windows - ESP-IDF 5.5.x

1. Instale/abra o terminal **ESP-IDF 5.5.x PowerShell**.
2. Clone o repositorio (na primeira vez):
   `git clone https://github.com/JuniorNarciso26/JrRobot.git`
3. Entre na pasta:
   `cd JrRobot`
4. Baixe as atualizacoes e selecione esta branch:
   `git fetch origin`
   `git switch v2-revisada`
   `git pull origin v2-revisada`
5. Entre no firmware:
   `cd firmware`
6. Configure o alvo:
   `idf.py set-target esp32s3`
7. Compile:
   `idf.py build`
8. Conecte a placa por USB e descubra a porta COM no Gerenciador de Dispositivos.
9. Grave, trocando COM5 pela sua porta:
   `idf.py -p COM5 flash`
10. Abra o monitor serial:
   `idf.py -p COM5 monitor`
11. Para sair do monitor: `Ctrl+]`.

## Atualizacoes futuras desta branch

Dentro da pasta JrRobot:
`git switch v2-revisada`
`git pull origin v2-revisada`
`cd firmware`
`idf.py build`
`idf.py -p COM5 flash monitor`

## Antes de gravar

- Confirme que a placa e ESP32-S3.
- OLED V2: SDA GPIO1, SCL GPIO2.
- Comece pelo perfil face_only; audio/camera devem ser habilitados somente depois da validacao fisica da pinagem.
- Nao altere eFuses para testar esta versao.

## Se a placa nao entrar em modo de gravacao

Mantenha BOOT pressionado, pressione e solte RESET/EN, solte BOOT e execute novamente o comando `idf.py -p COMx flash`.

## Voltar para main

`git switch main`
`git pull origin main`
`cd firmware`
`idf.py set-target esp32s3`
`idf.py build`
`idf.py -p COM5 flash monitor`
