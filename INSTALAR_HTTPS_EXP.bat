@echo off
setlocal EnableExtensions
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================
echo JrBot - Experimento HTTPS local EXP 02
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
echo ========================================
echo ETAPA IMPORTANTE - CONFIAR NA CA LOCAL
echo ========================================
echo Arquivo publico da CA:
echo   %CD%\firmware\certs\local\jrbot-dev-ca.pem
echo.
echo A chave privada da CA NAO deve ser copiada nem instalada em outros dispositivos.
echo Para o primeiro teste no Windows, importe jrbot-dev-ca.pem em
echo "Autoridades de Certificacao Raiz Confiaveis" do usuario atual.
echo Depois feche e reabra o navegador.
echo.
echo [INFO] Compilando e gravando JrBot_HTTPS_EXP_02...
set "JRBOT_SYNC_DONE=1"
call INSTALAR.bat flash
if errorlevel 1 goto falha

echo.
echo ========================================
echo [SUCESSO] EXPERIMENTO HTTPS GRAVADO
echo ========================================
echo Firmware esperado: JrBot_HTTPS_EXP_02
echo.
echo Recuperacao HTTP:
echo   http://%JRBOT_HTTPS_IP%/https-test
echo.
echo Teste HTTPS:
echo   https://%JRBOT_HTTPS_IP%/https-test
echo.
echo Para o teste de microfone, a barra do navegador deve deixar de indicar
echo certificado inseguro depois de confiar na CA local.
echo Confira na pagina: Secure Context, mediaDevices, getUserMedia e MediaRecorder.
echo.
pause
exit /b 0

:falha
echo.
echo [FALHA] Experimento HTTPS nao foi instalado.
echo Nenhuma alteracao foi feita na branch v1.
pause
exit /b 1
