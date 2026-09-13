@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

rem ============================================================
rem JRBOT - ATUALIZADOR / BUILD / FLASH / PAINEL
rem ============================================================
rem Baseado no bootstrap do Locker_Cpgas: o BAT roda por uma copia
rem temporaria para poder atualizar/trocar a propria branch com seguranca.

if /i "%JRBOT_SYNC_DONE%"=="1" goto local_entry

if /i not "%~1"=="--runner" (
    set "RUNNER=%TEMP%\JrBot_Instalar_Runner.bat"
    copy /y "%~f0" "%TEMP%\JrBot_Instalar_Runner.bat" >nul
    call "%TEMP%\JrBot_Instalar_Runner.bat" --runner "%~dp0" "%~1" "%~2"
    exit /b !ERRORLEVEL!
)

set "REPO_URL=https://github.com/JuniorNarciso26/JrRobot.git"
set "PROJECT_DIR=%~2"
set "ACTION=%~3"
set "JR_FLASH_PORT=%~4"
if "%ACTION%"=="" set "ACTION=all"
set "CLONE_TEMP=%TEMP%\JrRobot_update"
set "STATUS_TMP=%TEMP%\jrbot_git_status.txt"

cd /d "%PROJECT_DIR%"
title JrBot - Atualizador e Instalador
cls
echo ============================================================
echo   JRBOT - ESCOLHA A VERSAO PARA INSTALAR
 echo ============================================================
echo.
echo O instalador consulta o GitHub, atualiza a branch escolhida,
echo compila, grava o ESP32 e abre o painel de desenvolvimento.
echo.

call :ensure_git
if errorlevel 1 goto updater_failure
call :select_and_sync
if errorlevel 1 goto updater_failure

set "JRBOT_SYNC_DONE=1"
call "%PROJECT_DIR%INSTALAR.bat" "%ACTION%" "%JR_FLASH_PORT%"
exit /b %ERRORLEVEL%

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

:choose_branch
set "TARGET_BRANCH="
set "CHANNEL="
echo.
echo Escolha a versao/canal:
if defined CURRENT_BRANCH echo   [1] Branch atual: !CURRENT_BRANCH!
echo   [2] main    - versao principal/estavel
echo   [3] develop - versao de integracao/desenvolvimento
echo   [4] feature/v1-internal-web-panel - JrBot V1 em teste
echo   [5] Digitar outra branch
echo.
if defined CURRENT_BRANCH (
    set /p "CHANNEL=Opcao [1-5] (ENTER = atual): "
    if "!CHANNEL!"=="" set "CHANNEL=1"
    if "!CHANNEL!"=="1" set "TARGET_BRANCH=!CURRENT_BRANCH!"
) else (
    set /p "CHANNEL=Opcao [2-5]: "
)
if "!CHANNEL!"=="2" set "TARGET_BRANCH=main"
if "!CHANNEL!"=="3" set "TARGET_BRANCH=develop"
if "!CHANNEL!"=="4" set "TARGET_BRANCH=feature/v1-internal-web-panel"
if "!CHANNEL!"=="5" set /p "TARGET_BRANCH=Nome completo da branch: "
if not defined TARGET_BRANCH (
    echo [ERRO] Opcao invalida.
    exit /b 1
)
git check-ref-format --branch "!TARGET_BRANCH!" >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Nome de branch invalido: !TARGET_BRANCH!
    exit /b 1
)
exit /b 0

:select_and_sync
echo.
echo ============================================================
echo [GITHUB] Sincronizando JrBot
 echo ============================================================

if exist "%PROJECT_DIR%\.git\" goto existing_repo

set "CURRENT_BRANCH="
call :choose_branch
if errorlevel 1 exit /b 1

echo [INFO] Primeira instalacao. Baixando %TARGET_BRANCH%...
git ls-remote --exit-code --heads "%REPO_URL%" "%TARGET_BRANCH%" >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Branch nao encontrada no GitHub: %TARGET_BRANCH%
    exit /b 1
)
if exist "%CLONE_TEMP%" rmdir /s /q "%CLONE_TEMP%"
git clone --branch "%TARGET_BRANCH%" --single-branch "%REPO_URL%" "%CLONE_TEMP%"
if errorlevel 1 exit /b 1
xcopy "%CLONE_TEMP%\*" "%PROJECT_DIR%" /E /H /K /Y /I >nul
if errorlevel 1 (
    rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
    echo [ERRO] Falha copiando o projeto.
    exit /b 1
)
rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
echo [OK] Projeto baixado. Branch: %TARGET_BRANCH%
exit /b 0

:existing_repo
pushd "%PROJECT_DIR%"
for /f "delims=" %%B in ('git branch --show-current') do set "CURRENT_BRANCH=%%B"
if not defined CURRENT_BRANCH (
    echo [ERRO] Repositorio em detached HEAD.
    popd
    exit /b 1
)

rem O runner temporario permite restaurar apenas o instalador antes de
rem verificar alteracoes locais, seguindo o mesmo principio do Locker_Cpgas.
git restore --source=HEAD --worktree -- "INSTALAR.bat" >nul 2>&1
if errorlevel 1 git checkout -- "INSTALAR.bat" >nul 2>&1

git status --porcelain > "%STATUS_TMP%"
for %%A in ("%STATUS_TMP%") do set "STATUS_SIZE=%%~zA"
if not "%STATUS_SIZE%"=="0" (
    echo [ERRO] Existem alteracoes locais. Nada sera sobrescrito.
    type "%STATUS_TMP%"
    del "%STATUS_TMP%" >nul 2>&1
    popd
    exit /b 1
)
if exist "%STATUS_TMP%" del "%STATUS_TMP%" >nul 2>&1

git fetch --prune origin
if errorlevel 1 (
    echo [ERRO] Nao foi possivel consultar o GitHub.
    popd
    exit /b 1
)

call :choose_branch
if errorlevel 1 (
    popd
    exit /b 1
)

git show-ref --verify --quiet "refs/remotes/origin/%TARGET_BRANCH%"
if errorlevel 1 (
    echo [ERRO] Branch remota nao encontrada: %TARGET_BRANCH%
    popd
    exit /b 1
)

if /i "%TARGET_BRANCH%"=="%CURRENT_BRANCH%" goto update_target

echo [INFO] Trocando %CURRENT_BRANCH% -^> %TARGET_BRANCH%...
git show-ref --verify --quiet "refs/heads/%TARGET_BRANCH%"
if errorlevel 1 (
    git switch --track -c "%TARGET_BRANCH%" "origin/%TARGET_BRANCH%"
) else (
    git switch "%TARGET_BRANCH%"
)
if errorlevel 1 (
    echo [ERRO] Nao foi possivel trocar de branch.
    popd
    exit /b 1
)

:update_target
git merge --ff-only "origin/%TARGET_BRANCH%"
if errorlevel 1 (
    echo [ERRO] A branch local divergiu do GitHub.
    echo Nenhum reset automatico sera realizado.
    popd
    exit /b 1
)
for /f "delims=" %%C in ('git rev-parse --short HEAD') do echo [OK] Branch %TARGET_BRANCH% atualizada. Commit: %%C
popd
exit /b 0

:updater_failure
echo.
echo ============================================================
echo [FALHA] Nao foi possivel atualizar o projeto.
 echo ============================================================
echo Nenhum reset automatico foi executado.
pause
exit /b 1

rem ============================================================
rem EXECUCAO LOCAL DA BRANCH SELECIONADA
rem ============================================================
:local_entry
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"
if not "%~2"=="" set "JR_FLASH_PORT=%~2"
set "BUILD_DIR=build-runtime-api-v1-02"
set "SDKCONFIG_FILE=sdkconfig.runtime-api-v1-02"
set "PORT_FILE=%TEMP%\jrbot_port_%RANDOM%_%RANDOM%.txt"
cd /d "%~dp0"

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
python tools\select_port.py --output "%PORT_FILE%"
if errorlevel 1 exit /b 1
set /p JR_FLASH_PORT=<"%PORT_FILE%"
del /q "%PORT_FILE%" >nul 2>&1
if not defined JR_FLASH_PORT exit /b 1
echo Porta escolhida: %JR_FLASH_PORT%
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
python tests\hardware\test_pin_policy.py
if errorlevel 1 exit /b 1
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
echo [SUCESSO] JrBot preparado e gravado.
exit /b 0

:failure
echo.
echo [FALHA] Processo interrompido. Nada deve ser gravado apos erro de build.
pause
exit /b 1

:help
echo.
echo JrBot - instalador/atualizador
echo.
echo   INSTALAR.bat              Escolhe branch, atualiza, compila, grava e abre o painel
echo   INSTALAR.bat build        Escolhe branch, atualiza e compila
echo   INSTALAR.bat flash        Escolhe branch, atualiza, compila e grava
echo   INSTALAR.bat panel        Escolhe branch, atualiza e abre o painel
echo.
exit /b 0
