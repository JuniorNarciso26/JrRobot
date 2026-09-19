@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

rem ============================================================
rem JRBOT - ATUALIZADOR / BUILD / FLASH / PAINEL
rem O menu lista dinamicamente todas as branches remotas ativas
rem ============================================================

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
echo Branches ativas sao consultadas diretamente no GitHub.
echo Branches archive/* ficam ocultas do menu.
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
    set "REMOTE_REF=%%R"
    set "REMOTE_BRANCH=!REMOTE_REF:refs/heads/=!"
    if /i not "!REMOTE_BRANCH:~0,8!"=="archive/" (
        set /a BRANCH_COUNT+=1
        set "BRANCH_!BRANCH_COUNT!=!REMOTE_BRANCH!"
    )
)
if !BRANCH_COUNT! LEQ 0 (
    echo [ERRO] Nenhuma branch ativa foi encontrada no GitHub.
    exit /b 1
)
echo [OK] !BRANCH_COUNT! branch(es) ativa(s) disponivel(is).
exit /b 0

:is_current_active
set "CURRENT_ACTIVE=0"
if not defined CURRENT_BRANCH exit /b 0
for /l %%N in (1,1,!BRANCH_COUNT!) do (
    if /i "!BRANCH_%%N!"=="!CURRENT_BRANCH!" set "CURRENT_ACTIVE=1"
)
exit /b 0

:choose_branch
set "TARGET_BRANCH="
set "CHANNEL="
echo.
echo [GITHUB] Consultando branches ativas...
call :refresh_branch_list
if errorlevel 1 exit /b 1
call :is_current_active

echo.
echo ============================================================
echo   ESCOLHA A VERSAO
echo ============================================================
echo.
for /l %%N in (1,1,!BRANCH_COUNT!) do (
    set "DISPLAY_BRANCH=!BRANCH_%%N!"
    set "BRANCH_NOTE="
    if /i "!DISPLAY_BRANCH!"=="main" set "BRANCH_NOTE= - ultima versao aprovada"
    if /i "!DISPLAY_BRANCH!"=="develop" set "BRANCH_NOTE= - integracao"
    if /i "!DISPLAY_BRANCH!"=="v1" set "BRANCH_NOTE= - JrBot V1"
    if /i "!DISPLAY_BRANCH!"=="v2" set "BRANCH_NOTE= - JrBot V2 voz"
    if /i "!DISPLAY_BRANCH:~0,8!"=="feature/" set "BRANCH_NOTE= - desenvolvimento / teste"
    if /i "!DISPLAY_BRANCH:~0,7!"=="hotfix/" set "BRANCH_NOTE= - correcao"
    if /i "!DISPLAY_BRANCH:~0,4!"=="fix/" set "BRANCH_NOTE= - correcao"
    if defined CURRENT_BRANCH if /i "!DISPLAY_BRANCH!"=="!CURRENT_BRANCH!" set "BRANCH_NOTE=!BRANCH_NOTE! [ATUAL]"
    echo   [%%N] !DISPLAY_BRANCH!!BRANCH_NOTE!
)
echo.

if "!CURRENT_ACTIVE!"=="1" (
    set /p "CHANNEL=Digite o numero da versao (ENTER = !CURRENT_BRANCH!): "
    if "!CHANNEL!"=="" (
        set "TARGET_BRANCH=!CURRENT_BRANCH!"
        goto branch_selected
    )
) else (
    if defined CURRENT_BRANCH echo [INFO] Branch antiga detectada: !CURRENT_BRANCH!
    set /p "CHANNEL=Digite o numero da versao: "
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
    echo [ERRO] Nenhuma versao selecionada.
    exit /b 1
)
echo [OK] Versao selecionada: !TARGET_BRANCH!
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
    echo [ERRO] Nao foi possivel baixar !TARGET_BRANCH!.
    exit /b 1
)
xcopy "%CLONE_TEMP%\*" "%PROJECT_DIR%" /E /H /K /Y /I >nul
if errorlevel 1 (
    rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
    echo [ERRO] Falha copiando o projeto.
    exit /b 1
)
rmdir /s /q "%CLONE_TEMP%" >nul 2>&1
echo [OK] Projeto baixado. Versao: !TARGET_BRANCH!
exit /b 0

:existing_repo
pushd "%PROJECT_DIR%"
for /f "delims=" %%B in ('git branch --show-current') do set "CURRENT_BRANCH=%%B"
if not defined CURRENT_BRANCH (
    echo [ERRO] Repositorio em detached HEAD.
    popd
    exit /b 1
)

rem O runner temporario permite restaurar o instalador antes da verificacao.
git restore --source=HEAD --worktree -- "INSTALAR.bat" >nul 2>&1
if errorlevel 1 git checkout -- "INSTALAR.bat" >nul 2>&1

git status --porcelain > "!STATUS_TMP!"
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
    echo [ERRO] Versao remota nao encontrada: !TARGET_BRANCH!
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
    echo [ERRO] Nao foi possivel trocar de versao.
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
for /f "delims=" %%C in ('git rev-parse --short HEAD') do echo [OK] !TARGET_BRANCH! atualizada. Commit: %%C
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
rem EXECUCAO LOCAL DA VERSAO SELECIONADA
rem ============================================================
:local_entry
set "PYTHONUTF8=1"
set "PYTHONIOENCODING=utf-8"
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"
if not "%~2"=="" set "JR_FLASH_PORT=%~2"
set "PORT_FILE=%TEMP%\jrbot_port_%RANDOM%_%RANDOM%.txt"
cd /d "%~dp0"

set "CURRENT_BRANCH="
for /f "delims=" %%B in ('git branch --show-current 2^>nul') do set "CURRENT_BRANCH=%%B"

if /i "%CURRENT_BRANCH%"=="v2" (
    set "BUILD_DIR=build-runtime-api-v1-05"
    set "SDKCONFIG_FILE=sdkconfig.runtime-api-v1-05"
) else (
    set "BUILD_DIR=build-runtime-api-v1-02"
    set "SDKCONFIG_FILE=sdkconfig.runtime-api-v1-02"
)

if /i "%ACTION%"=="help" goto help
if /i "%ACTION%"=="panel" goto panel

call :ensure_idf
if errorlevel 1 goto failure

if /i "%ACTION%"=="build" goto prepare
if /i "%ACTION%"=="flash" goto select_then_prepare
if /i "%ACTION%"=="all" goto select_then_prepare
if /i "%ACTION%"=="menuconfig" goto menuconfig

echo ERRO: opcao invalida: %ACTION%
goto help

:ensure_idf
if defined IDF_PATH if exist "%IDF_PATH%\tools\idf.py" goto idf_ok
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
echo ============================================================
echo [SUCESSO] JRBOT PREPARADO
echo ============================================================
if defined CURRENT_BRANCH echo Branch: %CURRENT_BRANCH%
if exist firmware\version.txt (
    set /p FW_VERSION=<firmware\version.txt
    echo Firmware: !FW_VERSION!
)
if defined JR_FLASH_PORT echo Porta: %JR_FLASH_PORT%
exit /b 0

:failure
if exist "%PORT_FILE%" del /q "%PORT_FILE%" >nul 2>&1
echo.
echo ============================================================
echo [FALHA] Processo interrompido.
echo ============================================================
echo Nenhum reset automatico do repositorio foi executado.
pause
exit /b 1

:help
echo.
echo JrBot - instalador/atualizador
echo.
echo   INSTALAR.bat              escolhe versao, atualiza, compila, grava e abre painel
echo   INSTALAR.bat build        escolhe versao, atualiza e compila
echo   INSTALAR.bat flash        escolhe versao, atualiza, compila e grava
echo   INSTALAR.bat panel        escolhe versao, atualiza e abre painel
echo   INSTALAR.bat menuconfig   escolhe versao, atualiza e abre menuconfig
echo.
echo O menu consulta todas as branches remotas ativas no GitHub.
echo Branches archive/* nao aparecem no menu.
echo.
exit /b 0
