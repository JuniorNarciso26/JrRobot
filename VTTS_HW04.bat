@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"
cd /d "%~dp0"

set "ACTION=%~1"
set "PORT=%~2"
if /i "%ACTION%"=="panel" set "ACTION=monitor"
if /i "%ACTION%"=="help" goto help
if not defined ACTION goto menu
goto execute

:menu
echo.
echo ==========================================================
echo   JRBOT V2 - TESTE TTS PORTUGUES OFFLINE - HW04
echo   Branch: test/vtts-pt-br-hw04
echo   Firmware principal NAO e alterado no GitHub
echo ==========================================================
echo 1 - Verificar ferramentas (doctor)
echo 2 - Preparar / baixar vTTS + modelos pt-BR
echo 3 - Preparar e COMPILAR (sem gravar)
echo 4 - Preparar, compilar e GRAVAR na ESP32
echo 5 - Abrir monitor Serial
echo 6 - Preparar, compilar, gravar e abrir Serial
echo 0 - Sair
echo.
set /p "CHOICE=Escolha: "
if "%CHOICE%"=="1" set "ACTION=doctor"
if "%CHOICE%"=="2" set "ACTION=prepare"
if "%CHOICE%"=="3" set "ACTION=build"
if "%CHOICE%"=="4" set "ACTION=flash"
if "%CHOICE%"=="5" set "ACTION=monitor"
if "%CHOICE%"=="6" set "ACTION=all"
if "%CHOICE%"=="0" exit /b 0
if not defined ACTION goto menu
goto execute

:execute
rem A instalacao da ESP-IDF 6.1 e separada da 5.5.x usada no JrBot V2.0.
rem Prefira executar dentro do terminal IDF_PowerShell 6.1 do EIM.
if not defined IDF_PATH (
    for /d %%D in ("%USERPROFILE%\esp\v6.1*" "C:\Espressif\frameworks\esp-idf-v6.1*") do (
        if exist "%%~fD\export.bat" call "%%~fD\export.bat"
    )
)
where python.exe >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Python nao encontrado. Abra o terminal do ESP-IDF 6.1.
    exit /b 1
)
python "%~dp0experiments\vtts-hw04\run.py" "%ACTION%" %PORT%
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" echo [ERRO] Teste nao concluido. Consulte a mensagem acima.
exit /b %RC%

:help
echo VTTS_HW04.bat [doctor ^| prepare ^| build ^| flash ^| monitor ^| all] [COMx]
echo Exemplo: VTTS_HW04.bat all COM8
echo Requer ESP-IDF 6.1 no Windows. O teste nao altera a branch main.
exit /b 0
