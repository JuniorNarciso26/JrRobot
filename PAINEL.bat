@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Painel de Manutencao
echo ========================================
echo Feche qualquer programa que esteja usando a porta da placa.
echo O Painel de Manutencao abre no navegador apos o servidor iniciar.
echo Para encerrar o painel, volte a esta janela e pressione ENTER.
echo.
where python >nul 2>nul
if errorlevel 1 goto sem_python
python -c "import serial" >nul 2>nul
if errorlevel 1 (
  python -m pip install -r "%~dp0tools\jrbot_frontend\requirements.txt"
  if errorlevel 1 goto falha
)
python "%~dp0tools\jrbot_frontend\run_panel_network.py"
if errorlevel 1 goto falha
exit /b 0

:sem_python
echo Python nao encontrado. Abra este arquivo pelo terminal ESP-IDF instalado.
goto falha

:falha
echo Nao foi possivel iniciar. Leia a mensagem acima. Nao abra outro painel em paralelo.
pause
exit /b 1
