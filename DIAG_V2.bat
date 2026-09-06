@echo off
setlocal
if not defined IDF_PATH (
    echo ERRO: abra no terminal ESP-IDF 5.5.x.
    exit /b 1
)
if not exist "%IDF_PATH%\tools\idf.py" exit /b 1
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=build"
if /i not "%ACTION%"=="build" if /i not "%ACTION%"=="flash" if /i not "%ACTION%"=="menuconfig" (
    echo Uso: DIAG_V2.bat build ^| flash ^| menuconfig
    echo Para operar, abra PAINEL.bat e selecione COM4. Nao e necessario monitor.
    exit /b 1
)
if not defined JR_FLASH_PORT set "JR_FLASH_PORT=COM6"
pushd "%~dp0" || exit /b 1
python tools\generate_pinmap.py --check
if errorlevel 1 goto failure
cd firmware
findstr /x /c:"JRBOT-V2-DIAG-02" version.txt >nul 2>nul
if errorlevel 1 goto failure
echo Projeto: %CD%
echo Firmware: JRBOT-V2-DIAG-02 / hardware HW03
if /i "%ACTION%"=="menuconfig" goto menuconfig
python "%IDF_PATH%\tools\idf.py" -B build-hw03 -D SDKCONFIG=sdkconfig.hw03 build
if errorlevel 1 goto failure
findstr /x /c:"CONFIG_JR_HEADLESS_DIAGNOSTIC=y" sdkconfig.hw03 >nul
if errorlevel 1 (
    echo ERRO: mantenha Run without OLED habilitado nesta fase.
    goto failure
)
if /i "%ACTION%"=="build" goto success
echo Gravando em %JR_FLASH_PORT%. Desconecte o painel durante a gravacao.
python "%IDF_PATH%\tools\idf.py" -B build-hw03 -D SDKCONFIG=sdkconfig.hw03 -p "%JR_FLASH_PORT%" flash
if errorlevel 1 goto failure
echo Gravacao concluida. Abra PAINEL.bat e conecte a COM4.
goto success
:menuconfig
python "%IDF_PATH%\tools\idf.py" -B build-hw03 -D SDKCONFIG=sdkconfig.hw03 menuconfig
if errorlevel 1 goto failure
:success
popd
exit /b 0
:failure
echo ERRO: processo interrompido. Nao prossiga com outro build ou pinagem antiga.
popd
exit /b 1
