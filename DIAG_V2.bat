@echo off
setlocal
if not defined IDF_PATH (
    echo ERRO: abra este comando no terminal ESP-IDF 5.5.x.
    exit /b 1
)
if not exist "%IDF_PATH%\tools\idf.py" (
    echo ERRO: IDF_PATH nao aponta para um ESP-IDF valido.
    exit /b 1
)
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=build"
if /i not "%ACTION%"=="build" if /i not "%ACTION%"=="flash" if /i not "%ACTION%"=="monitor" if /i not "%ACTION%"=="menuconfig" (
    echo Uso: DIAG_V2.bat build ^| flash ^| monitor ^| menuconfig
    exit /b 1
)
pushd "%~dp0firmware" || exit /b 1
findstr /x /c:"JRBOT-V2-DIAG-01" version.txt >nul 2>nul
if errorlevel 1 (
    echo ERRO: marcador JRBOT-V2-DIAG-01 ausente. Nao gravar codigo antigo.
    popd
    exit /b 1
)
echo Projeto: %CD%
echo Versao esperada: JRBOT-V2-DIAG-01
if /i "%ACTION%"=="monitor" goto monitor
if /i "%ACTION%"=="menuconfig" goto menuconfig
python "%IDF_PATH%\tools\idf.py" -B build-diag -D SDKCONFIG=sdkconfig.diag build
if errorlevel 1 goto failure
findstr /x /c:"CONFIG_JR_HEADLESS_DIAGNOSTIC=y" sdkconfig.diag >nul
if errorlevel 1 (
    echo ERRO: este roteiro exige HEADLESS habilitado em DIAG_V2.bat menuconfig.
    goto failure
)
if /i "%ACTION%"=="build" goto success
rem Flash rebuilds and checks the selected configuration before writing anything.
echo Gravando na COM6. Feche outros programas que utilizem essa porta.
python "%IDF_PATH%\tools\idf.py" -B build-diag -D SDKCONFIG=sdkconfig.diag -p COM6 flash
if errorlevel 1 goto failure
echo Gravacao concluida. Use DIAG_V2.bat monitor para abrir a COM4.
goto success
:monitor
python "%IDF_PATH%\tools\idf.py" -B build-diag -D SDKCONFIG=sdkconfig.diag -p COM4 monitor
if errorlevel 1 goto failure
goto success
:menuconfig
python "%IDF_PATH%\tools\idf.py" -B build-diag -D SDKCONFIG=sdkconfig.diag menuconfig
if errorlevel 1 goto failure
:success
popd
exit /b 0
:failure
echo ERRO: processo interrompido. Nao prossiga com arquivos de outro build.
popd
exit /b 1
