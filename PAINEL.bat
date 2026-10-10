@echo off
setlocal EnableExtensions
cd /d "%~dp0"
echo ========================================
echo JrBot - Painel de desenvolvimento
echo ========================================
echo Feche qualquer programa que esteja usando a porta da placa.
echo O navegador abre apos o servidor iniciar.
echo Para encerrar o painel, volte a esta janela e pressione ENTER.
echo.

rem IMPORTANTE:
rem O painel usa dependencias Python diferentes das dependencias do ESP-IDF.
rem Nunca instalar requirements do painel dentro do python_env do ESP-IDF.
set "PANEL_VENV=%LOCALAPPDATA%\JrBot\panel-venv"
set "PANEL_PY=%PANEL_VENV%\Scripts\python.exe"

if exist "%PANEL_PY%" goto panel_python_ready

where py.exe >nul 2>nul
if errorlevel 1 goto sem_python_sistema

echo [INFO] Criando ambiente Python isolado do painel...
if not exist "%LOCALAPPDATA%\JrBot" mkdir "%LOCALAPPDATA%\JrBot" >nul 2>nul
py -3 -m venv "%PANEL_VENV%"
if errorlevel 1 goto falha

:panel_python_ready
"%PANEL_PY%" -c "import serial; from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey; from solders.transaction import Transaction; import importlib.metadata; assert importlib.metadata.version('cryptography') == '50.0.1'; assert importlib.metadata.version('solders') == '0.29.0'" >nul 2>nul
if errorlevel 1 (
  echo [INFO] Instalando dependencias somente no ambiente isolado do painel...
  "%PANEL_PY%" -m pip install -r "%~dp0tools\jrbot_frontend\requirements.txt"
  if errorlevel 1 goto falha
)

"%PANEL_PY%" "%~dp0tools\jrbot_frontend\run_panel_network.py"
if errorlevel 1 goto falha
exit /b 0

:sem_python_sistema
echo Python do sistema nao encontrado pelo launcher py.exe.
echo Instale Python 3 para Windows. O painel nao deve usar o Python interno do ESP-IDF.
goto falha

:falha
echo Nao foi possivel iniciar. Leia a mensagem acima. Nao abra outro painel em paralelo.
pause
exit /b 1
