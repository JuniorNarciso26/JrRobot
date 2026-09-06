@echo off
setlocal
cd /d "%~dp0"

echo.
echo ========================================
echo TESTA_FIO - JrBot ESP32-S3
echo ========================================
echo.
echo Este teste grava um firmware temporario na placa.
echo Ele testa somente GPIO 1, 2, 41, 42, 47 e 21.
echo Depois do teste, rode INSTALAR.bat para voltar ao firmware normal do JrBot.
echo.

if not defined IDF_PATH (
  echo ERRO: abra este arquivo pelo terminal ESP-IDF 5.5.x.
  pause
  exit /b 1
)
if not exist "%IDF_PATH%\tools\idf.py" (
  echo ERRO: IDF_PATH invalido: %IDF_PATH%
  pause
  exit /b 1
)

python -c "import serial" >nul 2>nul
if errorlevel 1 (
  echo Instalando pyserial...
  python -m pip install pyserial
  if errorlevel 1 goto failure
)

set "PORT_FILE=%TEMP%\jrbot_wire_port_%RANDOM%_%RANDOM%.txt"
python tools\select_port.py --output "%PORT_FILE%"
if errorlevel 1 goto failure
set /p WIRE_PORT=<"%PORT_FILE%"
del /q "%PORT_FILE%" >nul 2>nul
if not defined WIRE_PORT (
  echo ERRO: nenhuma porta foi selecionada.
  goto failure
)

echo.
echo Porta escolhida para gravacao: %WIRE_PORT%
echo Compilando firmware TESTA_FIO...

pushd wire_test
python "%IDF_PATH%\tools\idf.py" -B build build
if errorlevel 1 (
  popd
  goto failure
)

echo.
echo Gravando TESTA_FIO em %WIRE_PORT%...
python "%IDF_PATH%\tools\idf.py" -B build -p "%WIRE_PORT%" flash
if errorlevel 1 (
  popd
  goto failure
)
popd

echo.
echo Firmware de teste gravado.
echo Aguardando a placa reiniciar...
timeout /t 2 /nobreak >nul

echo.
python tools\testa_fio.py --port "%WIRE_PORT%"
set "RC=%errorlevel%"
echo.
echo Para restaurar o JrBot normal, execute: INSTALAR.bat
pause
exit /b %RC%

:failure
if exist "%PORT_FILE%" del /q "%PORT_FILE%" >nul 2>nul
echo.
echo ERRO - TESTA_FIO interrompido.
pause
exit /b 1
