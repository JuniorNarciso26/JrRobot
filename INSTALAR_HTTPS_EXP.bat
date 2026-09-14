@echo off
setlocal EnableExtensions
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================
echo JrBot - Experimento HTTPS local
echo ========================================
echo.

for /f "delims=" %%B in ('git branch --show-current 2^>nul') do set "CURRENT_BRANCH=%%B"
if /i not "%CURRENT_BRANCH%"=="experiment/https-local" (
  echo [ERRO] Branch atual: %CURRENT_BRANCH%
  echo Este instalador so pode ser usado em experiment/https-local.
  echo Execute:
  echo   git fetch origin
  echo   git switch experiment/https-local
  echo   git pull --ff-only origin experiment/https-local
  echo.
  pause
  exit /b 1
)

git status --porcelain > "%TEMP%\jrbot_https_status.txt"
for %%A in ("%TEMP%\jrbot_https_status.txt") do set "STATUS_SIZE=%%~zA"
if not "%STATUS_SIZE%"=="0" (
  echo [ERRO] Existem alteracoes locais. Nada sera sobrescrito.
  type "%TEMP%\jrbot_https_status.txt"
  del "%TEMP%\jrbot_https_status.txt" >nul 2>&1
  pause
  exit /b 1
)
del "%TEMP%\jrbot_https_status.txt" >nul 2>&1

echo [INFO] Atualizando branch experimental...
git fetch origin experiment/https-local
if errorlevel 1 goto falha
git merge --ff-only origin/experiment/https-local
if errorlevel 1 goto falha

echo.
set /p "JRBOT_HTTPS_IP=Digite o IPv4 ATUAL do JrBot para gerar o certificado: "
if "%JRBOT_HTTPS_IP%"=="" goto falha

python tools\generate_https_cert.py --ip "%JRBOT_HTTPS_IP%"
if errorlevel 1 goto falha

echo.
echo [INFO] Compilando e gravando JrBot_HTTPS_EXP_01...
set "JRBOT_SYNC_DONE=1"
call INSTALAR.bat flash
if errorlevel 1 goto falha

echo.
echo ========================================
echo [SUCESSO] EXPERIMENTO HTTPS GRAVADO
echo ========================================
echo Firmware esperado: JrBot_HTTPS_EXP_01
echo.
echo Primeiro teste de recuperacao:
echo   http://%JRBOT_HTTPS_IP%/https-test
echo.
echo Depois teste HTTPS:
echo   https://%JRBOT_HTTPS_IP%/https-test
echo.
echo Aceite o aviso do certificado apenas para este experimento.
echo Depois confira Secure Context, getUserMedia e MediaRecorder.
echo.
pause
exit /b 0

:falha
echo.
echo [FALHA] Experimento HTTPS nao foi instalado.
echo Nenhuma alteracao foi feita na branch v1.
pause
exit /b 1
