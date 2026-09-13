@echo off
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul

rem ============================================================
rem JRBOT - ATUALIZADOR / BUILD / FLASH / PAINEL
rem ============================================================
rem O primeiro BAT pode ser baixado sozinho. Ele executa uma copia
rem temporaria para poder sincronizar a propria pasta com o GitHub.

if /i not "%~1"=="--runner" (
    set "RUNNER=%TEMP%\JrBot_Instalar_Runner.bat"
    copy /y "%~f0" "%TEMP%\JrBot_Instalar_Runner.bat" >nul
    call "%TEMP%\JrBot_Instalar_Runner.bat" --runner "%~dp0" "%~1" "%~2"
    exit /b %ERRORLEVEL%
)

set "REPO_URL=https://github.com/JuniorNarciso26/JrRobot.git"
set "PROJECT_DIR=%~2"
set "ACTION=%~3"
set "JR_FLASH_PORT=%~4"
if "%ACTION%"=="" set "ACTION=all"

set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"
set "BUILD_DIR=build-runtime-api-v1-05"
set "SDKCONFIG_FILE=sdkconfig.runtime-api-v1-05"
set "CLONE_TEMP=%TEMP%\JrRobot_update"
set "STATUS_TMP=%TEMP%\jrbot_git_status.txt"
set "PORT_FILE=%TEMP%\jrbot_port_%RANDOM%_%RANDOM%.txt"

cd /d "%PROJECT_DIR%"

title JrBot - Atualizador e Instalador
cls
echo ============================================================
echo   JRBOT - ATUALIZADOR / BUILD / FLASH
 echo ============================================================
echo.
echo O instalador vai sincronizar o canal atual com o GitHub,
echo validar o projeto, compilar e, quando solicitado, gravar a placa.
echo.

if /i "%ACTION%"=="help" goto help

call :ensure_git
if errorlevel 1 goto failure

call :sync_github
if errorlevel 1 goto failure

rem O clone/atualizacao pode ter trazido scripts e arquivos novos.
cd /d "%PROJECT_DIR%"

if /i "%ACTION%"=="panel" goto panel

call :ensure_idf
if errorlevel 1 goto failure

if /i "%ACTION%"=="build" goto prepare
if /i "%ACTION%"=="flash" goto select_then_prepare
if /i "%ACTION%"=="all" goto select_then_prepare
if /i "%ACTION%"=="menuconfig" goto menuconfig

echo ERRO: opcao invalida: %ACTION%
goto help

:ensure_git
where git.exe >nul 2>&1
if not errorlevel 1 (
    echo [OK] Git encontrado.
    exit /b 0
)

echo [INFO] Git nao encontrado. Tentando instalar pelo winget...
where winget.exe >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Git nao esta instalado e o winget nao esta disponivel.
    exit /b 1
)
winget install --id Git.Git -e --accept-package-agreements --accept-source-agreements
if errorlevel 1 exit /b 1
set "PATH=%PATH%;%ProgramFiles%\Git\cmd;%ProgramFiles%\Git\bin"
where git.exe >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Git foi instalado, mas ainda nao foi localizado.
    echo Feche esta janela e execute INSTALAR.bat novamente.
    exit /b 1
)
echo [OK] Git instalado.
exit /b 0

:sync_github
echo.
echo ============================================================
echo [GITHUB] Sincronizando JrBot
 echo ============================================================

if exist "%PROJECT_DIR%\.git\" goto existing_repo

echo [INFO] Primeira execucao nesta pasta.
echo.
echo Escolha o canal inicial:
echo   [1] main    - baseline estavel promovida
echo   [2] develop - integracao da proxima versao
echo   [3] feature/voice-playground-v1 - candidata atual de bancada
echo.
set "BOOTSTRAP_BRANCH="
set /p "CHANNEL=Opcao [1-3]: "
if "%CHANNEL%"=="1" set "BOOTSTRAP_BRANCH=main"
if "%CHANNEL%"=="2" set "BOOTSTRAP_BRANCH=develop"
if "%CHANNEL%"=="3" set "BOOTSTRAP_BRANCH=feature/voice-playground-v1"
if not defined BOOTSTRAP_BRANCH (
    echo [ERRO] Canal invalido.
    exit /b 1
)

echo [INFO] Baixando %BOOTSTRAP_BRANCH% do GitHub...
if exist "%CLONE_TEMP%" rmdir /s /q "%CLONE_TEMP%"
git clone --branch "%BOOTSTRAP_BRANCH%" --single-branch "%REPO_URL%" "%CLONE_TEMP%"
if errorlevel 1 (
    echo [ERRO] Nao foi possivel clonar o JrBot.
    exit /b 1
)

xcopy "%CLONE_TEMP%\*" "%PROJECT_DIR%" /E /H /K /Y /I >nul
if errorlevel 1 (
    echo [ERRO] Falha copiando o clone para a pasta do JrBot.
    rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
    exit /b 1
)
rmdir /s /q "%CLONE_TEMP%" >nul 2>&1

echo [OK] Projeto baixado. Canal: %BOOTSTRAP_BRANCH%
exit /b 0

:existing_repo
pushd "%PROJECT_DIR%"
for /f "delims=" %%B in ('git branch --show-current') do set "CURRENT_BRANCH=%%B"
if not defined CURRENT_BRANCH (
    echo [ERRO] Repositorio em detached HEAD. Selecione main, develop, feature/* ou fix/*.
    popd
    exit /b 1
)

echo %CURRENT_BRANCH% | findstr /b /c:"archive/" >nul
if not errorlevel 1 (
    echo [ERRO] Branch archive/* e historica e nao deve receber desenvolvimento.
    popd
    exit /b 1
)

git remote get-url origin >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Remote origin nao configurado.
    popd
    exit /b 1
)

echo [INFO] Canal atual: %CURRENT_BRANCH%
git fetch origin "%CURRENT_BRANCH%"
if errorlevel 1 (
    echo [ERRO] Nao foi possivel consultar origin/%CURRENT_BRANCH%.
    popd
    exit /b 1
)

rem Como o runner temporario esta em execucao, o instalador local pode ser
rem restaurado antes da verificacao sem interromper este processo.
git restore --source=HEAD --worktree -- "INSTALAR.bat" >nul 2>&1
if errorlevel 1 git checkout -- "INSTALAR.bat" >nul 2>&1

git status --porcelain > "%STATUS_TMP%"
for %%A in ("%STATUS_TMP%") do set "STATUS_SIZE=%%~zA"
if not "%STATUS_SIZE%"=="0" (
    echo [ERRO] Existem alteracoes locais. Atualizacao cancelada para protege-las.
    echo.
    type "%STATUS_TMP%"
    del "%STATUS_TMP%" >nul 2>&1
    popd
    exit /b 1
)
if exist "%STATUS_TMP%" del "%STATUS_TMP%" >nul 2>&1

git merge --ff-only "origin/%CURRENT_BRANCH%"
if errorlevel 1 (
    echo [ERRO] O historico local divergiu do GitHub.
    echo Nenhum reset automatico sera feito. Revise a branch manualmente.
    popd
    exit /b 1
)
for /f "delims=" %%C in ('git rev-parse --short HEAD') do echo [OK] GitHub sincronizado. Commit: %%C
popd
exit /b 0

:ensure_idf
if defined IDF_PATH if exist "%IDF_PATH%\tools\idf.py" goto idf_ok

rem Tenta localizar instalacoes ESP-IDF 5.5.x comuns no Windows.
for /d %%D in ("%USERPROFILE%\esp\v5.5*") do if exist "%%~fD\export.bat" call "%%~fD\export.bat" >nul 2>&1
if defined IDF_PATH if exist "%IDF_PATH%\tools\idf.py" goto idf_ok
for /d %%D in ("C:\Espressif\frameworks\esp-idf-v5.5*") do if exist "%%~fD\export.bat" call "%%~fD\export.bat" >nul 2>&1
if defined IDF_PATH if exist "%IDF_PATH%\tools\idf.py" goto idf_ok

 echo [ERRO] ESP-IDF 5.5.x nao foi localizado.
echo Instale o ESP-IDF 5.5.x uma vez e execute INSTALAR.bat novamente.
exit /b 1

:idf_ok
echo [OK] ESP-IDF: %IDF_PATH%
exit /b 0

:select_then_prepare
call :select_port
if errorlevel 1 goto failure
goto prepare

:select_port
if defined JR_FLASH_PORT (
    echo Porta de gravacao definida: %JR_FLASH_PORT%
    exit /b 0
)
python tools\select_port.py --output "%PORT_FILE%"
if errorlevel 1 (
    if exist "%PORT_FILE%" del /q "%PORT_FILE%" >nul 2>&1
    exit /b 1
)
set /p JR_FLASH_PORT=<"%PORT_FILE%"
del /q "%PORT_FILE%" >nul 2>&1
if not defined JR_FLASH_PORT (
    echo ERRO: nenhuma porta foi selecionada.
    exit /b 1
)
echo Porta escolhida: %JR_FLASH_PORT%
exit /b 0

:check
python tools\generate_pinmap.py --check
if errorlevel 1 exit /b 1
python tests\hardware\test_pin_policy.py
if errorlevel 1 exit /b 1
findstr /b /c:"JRBotV2_" firmware\version.txt >nul 2>&1
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
echo Gravando JrBot + modelos ESP-SR em %JR_FLASH_PORT%...
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
call "%PROJECT_DIR%PAINEL.bat"
exit /b %errorlevel%

:success
echo.
echo ============================================================
echo [SUCESSO] JRBOT PREPARADO
 echo ============================================================
echo Firmware esperado nesta feature: JRBotV2_RUNTIME_API_V1_05
if defined CURRENT_BRANCH echo Branch: %CURRENT_BRANCH%
if defined JR_FLASH_PORT echo Hardware: JRBOT-HW-04 ^| Porta: %JR_FLASH_PORT% ^| Runtime API: 1.3
exit /b 0

:failure
if exist "%STATUS_TMP%" del "%STATUS_TMP%" >nul 2>&1
if exist "%PORT_FILE%" del "%PORT_FILE%" >nul 2>&1
echo.
echo ============================================================
echo [FALHA] Processo interrompido sem reset automatico do repositorio.
 echo ============================================================
echo Copie a mensagem acima para diagnostico.
pause
exit /b 1

:help
echo.
echo JrBot - instalador/atualizador
 echo.
echo   INSTALAR.bat              Atualiza, compila, grava e abre o painel
echo   INSTALAR.bat build        Atualiza e apenas compila
echo   INSTALAR.bat flash        Atualiza, compila e grava
echo   INSTALAR.bat flash COM7   Usa diretamente a COM7
echo   INSTALAR.bat panel        Atualiza e abre o painel
echo   INSTALAR.bat menuconfig   Atualiza e abre menuconfig
echo.
echo Branches seguem README/CONTRIBUTING: main, develop, feature/* e fix/*.
echo archive/* e somente historico.
echo.
exit /b 0
