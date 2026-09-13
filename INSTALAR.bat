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
    copy /y "%~f0" "!RUNNER!" >nul
    call "!RUNNER!" --runner "%~dp0" "%~1" "%~2"
    exit /b !ERRORLEVEL!
)

set "REPO_URL=https://github.com/JuniorNarciso26/JrRobot.git"
set "PROJECT_DIR=%~2"
set "ACTION=%~3"
set "JR_FLASH_PORT=%~4"
if "%ACTION%"=="" set "ACTION=all"
set "CLONE_TEMP=%TEMP%\JrRobot_update"
set "STATUS_TMP=%TEMP%\jrbot_git_status.txt"
set "STATUS_FILTERED_TMP=%TEMP%\jrbot_git_status_filtered.txt"

cd /d "%PROJECT_DIR%"
title JrBot - Atualizador e Instalador
cls
echo ============================================================
echo   JRBOT - ATUALIZADOR / INSTALADOR
 echo ============================================================
echo.
echo O instalador consulta o GitHub, carrega a lista atual de branches,
echo permite escolher a versao, atualiza, compila, grava o ESP32 e abre
echo o painel de desenvolvimento.
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

:refresh_branch_list
set "BRANCH_COUNT=0"
for /f "tokens=2" %%R in ('git ls-remote --heads "%REPO_URL%" 2^>nul') do (
    set "REMOTE_BRANCH=%%R"
    set "REMOTE_BRANCH=!REMOTE_BRANCH:refs/heads/=!"
    if /i not "!REMOTE_BRANCH:~0,8!"=="archive/" (
        set /a BRANCH_COUNT+=1
        set "BRANCH_!BRANCH_COUNT!=!REMOTE_BRANCH!"
    )
)
if !BRANCH_COUNT! LEQ 0 (
    echo [ERRO] Nao foi possivel obter a lista de branches ativas do GitHub.
    exit /b 1
)
echo [OK] Lista atualizada: !BRANCH_COUNT! branch(es) ativa(s).
exit /b 0

:choose_branch
set "TARGET_BRANCH="
set "CHANNEL="
echo.
echo [GITHUB] Atualizando lista de branches ativas...
call :refresh_branch_list
if errorlevel 1 exit /b 1

echo.
echo ============================================================
echo   ESCOLHA A VERSAO / BRANCH
 echo ============================================================
echo.
for /l %%N in (1,1,!BRANCH_COUNT!) do (
    set "DISPLAY_BRANCH=!BRANCH_%%N!"
    set "BRANCH_NOTE="
    if /i "!DISPLAY_BRANCH!"=="main" set "BRANCH_NOTE= - principal / estavel"
    if /i "!DISPLAY_BRANCH!"=="develop" set "BRANCH_NOTE= - integracao / desenvolvimento"
    if /i "!DISPLAY_BRANCH:~0,8!"=="feature/" set "BRANCH_NOTE= - feature em desenvolvimento"
    if /i "!DISPLAY_BRANCH:~0,4!"=="fix/" set "BRANCH_NOTE= - correcao em desenvolvimento"
    if defined CURRENT_BRANCH if /i "!DISPLAY_BRANCH!"=="!CURRENT_BRANCH!" set "BRANCH_NOTE=!BRANCH_NOTE! [ATUAL]"
    echo   [%%N] !DISPLAY_BRANCH!!BRANCH_NOTE!
)
echo.
if defined CURRENT_BRANCH (
    set /p "CHANNEL=Digite o numero da branch (ENTER = !CURRENT_BRANCH!): "
    if "!CHANNEL!"=="" (
        set "TARGET_BRANCH=!CURRENT_BRANCH!"
        goto branch_selected
    )
) else (
    set /p "CHANNEL=Digite o numero da branch: "
)

echo(!CHANNEL!| findstr /r "^[0-9][0-9]*$" >nul
if errorlevel 1 (
    echo [ERRO] Opcao invalida: !CHANNEL!
    exit /b 1
)
if !CHANNEL! LSS 1 (
    echo [ERRO] Opcao fora da lista.
    exit /b 1
)
if !CHANNEL! GTR !BRANCH_COUNT! (
    echo [ERRO] Opcao fora da lista.
    exit /b 1
)
for %%N in (!CHANNEL!) do set "TARGET_BRANCH=!BRANCH_%%N!"

:branch_selected
if not defined TARGET_BRANCH (
    echo [ERRO] Nenhuma branch selecionada.
    exit /b 1
)
git check-ref-format --branch "!TARGET_BRANCH!" >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Nome de branch invalido: !TARGET_BRANCH!
    exit /b 1
)
echo [OK] Branch selecionada: !TARGET_BRANCH!
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

echo [INFO] Primeira instalacao. Baixando !TARGET_BRANCH!...
if exist "%CLONE_TEMP%" rmdir /s /q "%CLONE_TEMP%"
git clone --branch "!TARGET_BRANCH!" --single-branch "%REPO_URL%" "%CLONE_TEMP%"
if errorlevel 1 (
    echo [ERRO] Nao foi possivel baixar a branch !TARGET_BRANCH!.
    exit /b 1
)
xcopy "%CLONE_TEMP%\*" "%PROJECT_DIR%" /E /H /K /Y /I >nul
if errorlevel 1 (
    rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
    echo [ERRO] Falha copiando o projeto.
    exit /b 1
)
rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
echo [OK] Projeto baixado. Branch: !TARGET_BRANCH!
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

rem O ESP-IDF Component Manager pode gerar firmware/dependencies.lock.
rem Enquanto ele estiver apenas como arquivo local nao rastreado, nao deve
rem impedir a atualizacao. Se um dia passar a ser rastreado e modificado,
rem a protecao normal do Git continua bloqueando a atualizacao.
git status --porcelain > "!STATUS_TMP!"
findstr /l /x /c:"?? firmware/dependencies.lock" "!STATUS_TMP!" >nul 2>&1
if not errorlevel 1 echo [INFO] Lockfile local do ESP-IDF detectado; ele nao bloqueia a atualizacao.
findstr /l /v /x /c:"?? firmware/dependencies.lock" "!STATUS_TMP!" > "!STATUS_FILTERED_TMP!"
for %%A in ("!STATUS_FILTERED_TMP!") do set "STATUS_SIZE=%%~zA"
if not "!STATUS_SIZE!"=="0" (
    echo [ERRO] Existem alteracoes locais. Nada sera sobrescrito.
    type "!STATUS_FILTERED_TMP!"
    del "!STATUS_TMP!" >nul 2>&1
    del "!STATUS_FILTERED_TMP!" >nul 2>&1
    popd
    exit /b 1
)
if exist "!STATUS_TMP!" del "!STATUS_TMP!" >nul 2>&1
if exist "!STATUS_FILTERED_TMP!" del "!STATUS_FILTERED_TMP!" >nul 2>&1

echo [INFO] Atualizando referencias remotas...
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

git show-ref --verify --quiet "refs/remotes/origin/!TARGET_BRANCH!"
if errorlevel 1 (
    echo [ERRO] Branch remota nao encontrada: !TARGET_BRANCH!
    popd
    exit /b 1
)

if /i "!TARGET_BRANCH!"=="!CURRENT_BRANCH!" goto update_target

echo [INFO] Trocando !CURRENT_BRANCH! -^> !TARGET_BRANCH!...
git show-ref --verify --quiet "refs/heads/!TARGET_BRANCH!"
if errorlevel 1 (
    git switch --track -c "!TARGET_BRANCH!" "origin/!TARGET_BRANCH!"
) else (
    git switch "!TARGET_BRANCH!"
)
if errorlevel 1 (
    echo [ERRO] Nao foi possivel trocar de branch.
    popd
    exit /b 1
)

:update_target
git merge --ff-only "origin/!TARGET_BRANCH!"
if errorlevel 1 (
    echo [ERRO] A branch local divergiu do GitHub.
    echo Nenhum reset automatico sera realizado.
    popd
    exit /b 1
)
for /f "delims=" %%C in ('git rev-parse --short HEAD') do echo [OK] Branch !TARGET_BRANCH! atualizada. Commit: %%C
popd
exit /b 0

:updater_failure
if exist "!STATUS_TMP!" del "!STATUS_TMP!" >nul 2>&1
if exist "!STATUS_FILTERED_TMP!" del "!STATUS_FILTERED_TMP!" >nul 2>&1
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
echo   INSTALAR.bat              Atualiza lista de branches, escolhe, compila, grava e abre o painel
echo   INSTALAR.bat build        Atualiza lista de branches, escolhe e compila
echo   INSTALAR.bat flash        Atualiza lista de branches, escolhe, compila e grava
echo   INSTALAR.bat panel        Atualiza lista de branches, escolhe e abre o painel
echo.
echo Branches archive/* sao historicas e nao aparecem no menu.
echo.
exit /b 0
