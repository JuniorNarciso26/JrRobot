@echo off
setlocal
cd /d "%~dp0"

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"
if not defined JR_FLASH_PORT set "JR_FLASH_PORT=COM6"
set "BUILD_DIR=build-final-hw04"
set "SDKCONFIG_FILE=sdkconfig.final-hw04"

if /i "%ACTION%"=="help" goto help
if /i "%ACTION%"=="build" goto prepare
if /i "%ACTION%"=="flash" goto prepare
if /i "%ACTION%"=="all" goto prepare
if /i "%ACTION%"=="menuconfig" goto menuconfig
if /i "%ACTION%"=="panel" goto panel

echo ERRO: opcao invalida: %ACTION%
goto help

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
echo Gravando JrBot em %JR_FLASH_PORT%...
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
echo OK - JrBot V2 atualizado.
echo Firmware: JRBotV2_2026-09-06-16:12
echo Hardware: JRBOT-HW-04 ^| Gravacao: COM6 ^| Painel: COM4
exit /b 0

:failure
echo.
echo ERRO - processo interrompido. Nada deve ser gravado apos uma falha de build.
exit /b 1

:help
echo.
echo JrBot V2 - instalador unico
echo.
echo   INSTALAR.bat            Compila, grava na COM6 e abre o painel estavel
echo   INSTALAR.bat build      Apenas compila
echo   INSTALAR.bat flash      Compila e grava
echo   INSTALAR.bat panel      Abre o painel estavel
echo   INSTALAR.bat menuconfig Abre configuracao do firmware
echo.
exit /b 0
