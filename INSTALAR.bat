@echo off
setlocal
cd /d "%~dp0"

rem O ESP-SR imprime caracteres Unicode ao empacotar os modelos.
rem No Windows, Python pode herdar cp1252 e falhar com UnicodeEncodeError.
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"
if not "%~2"=="" set "JR_FLASH_PORT=%~2"
set "BUILD_DIR=build-localbrain-voice"
set "SDKCONFIG_FILE=sdkconfig.localbrain-voice"

if /i "%ACTION%"=="help" goto help
if /i "%ACTION%"=="build" goto prepare
if /i "%ACTION%"=="flash" goto select_then_prepare
if /i "%ACTION%"=="all" goto select_then_prepare
if /i "%ACTION%"=="menuconfig" goto menuconfig
if /i "%ACTION%"=="panel" goto panel

echo ERRO: opcao invalida: %ACTION%
goto help

:select_then_prepare
call :select_port
if errorlevel 1 goto failure
goto prepare

:select_port
if defined JR_FLASH_PORT (
  echo Porta de gravacao definida: %JR_FLASH_PORT%
  exit /b 0
)
set "PORT_FILE=%TEMP%\jrbot_port_%RANDOM%_%RANDOM%.txt"
python tools\select_port.py --output "%PORT_FILE%"
if errorlevel 1 (
  if exist "%PORT_FILE%" del /q "%PORT_FILE%" >nul 2>nul
  exit /b 1
)
set /p JR_FLASH_PORT=<"%PORT_FILE%"
del /q "%PORT_FILE%" >nul 2>nul
if not defined JR_FLASH_PORT (
  echo ERRO: nenhuma porta foi selecionada.
  exit /b 1
)
echo Porta escolhida para esta instalacao: %JR_FLASH_PORT%
exit /b 0

:check
if not defined IDF_PATH (
  echo ERRO: abra este arquivo pelo terminal ESP-IDF 5.5.x.
  exit /b 1
)
if not exist "%IDF_PATH%\tools\idf.py" (
  echo ERRO: IDF_PATH invalido: %IDF_PATH%
  exit /b 1
)
python tools\generate_pinmap.py --check
if errorlevel 1 exit /b 1
findstr /b /c:"JRBotV2_" firmware\version.txt >nul 2>nul
if errorlevel 1 (
  echo ERRO: versao inesperada em firmware\version.txt.
  exit /b 1
)
exit /b 0

:prepare
call :check
if errorlevel 1 goto failure
pushd firmware
python "%IDF_PATH%\tools\idf.py" -B %BUILD_DIR% -D SDKCONFIG=%SDKCONFIG_FILE% build
if errorlevel 1 (
  popd
  goto failure
)
if /i "%ACTION%"=="build" (
  popd
  goto success
)
echo.
echo Gravando JrBot + modelo WakeNet em %JR_FLASH_PORT%...
python "%IDF_PATH%\tools\idf.py" -B %BUILD_DIR% -D SDKCONFIG=%SDKCONFIG_FILE% -p "%JR_FLASH_PORT%" flash
if errorlevel 1 (
  popd
  goto failure
)
popd
if /i "%ACTION%"=="flash" goto success
goto panel

:menuconfig
call :check
if errorlevel 1 goto failure
pushd firmware
python "%IDF_PATH%\tools\idf.py" -B %BUILD_DIR% -D SDKCONFIG=%SDKCONFIG_FILE% menuconfig
set "RC=%errorlevel%"
popd
if not "%RC%"=="0" goto failure
goto success

:panel
call "%~dp0PAINEL.bat"
exit /b %errorlevel%

:success
echo.
echo OK - JrBot Voice atualizado.
echo Firmware: JRBotV2_VOICE_01
if defined JR_FLASH_PORT echo Hardware: JRBOT-HW-04 ^| Gravacao: %JR_FLASH_PORT% ^| WakeNet: Hi ESP
exit /b 0

:failure
echo.
echo ERRO - processo interrompido. Nada deve ser gravado apos uma falha de build.
exit /b 1

:help
echo.
echo JrBot Voice - instalador com ESP-SR / WakeNet
echo.
echo   INSTALAR.bat              Escolhe a porta, compila, grava firmware + modelo e abre o painel
echo   INSTALAR.bat flash        Escolhe a porta, compila e grava firmware + modelo
echo   INSTALAR.bat flash COM7   Usa diretamente a COM7
echo   INSTALAR.bat build        Apenas compila e empacota o modelo
echo   INSTALAR.bat panel        Abre o painel com portas detectadas
echo   INSTALAR.bat menuconfig   Abre configuracao do firmware/ESP-SR
echo.
echo Wake word desta candidata: Hi ESP
echo O modelo personalizado JrBot sera trocado sem alterar a arquitetura do Local Brain.
echo.
exit /b 0
