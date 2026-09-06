@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Painel V2 / COM4
echo ========================================
echo Feche a janela do painel antigo e qualquer monitor usando COM4.
echo O navegador abre apos o servidor iniciar. Mantenha esta janela aberta.
echo COM6 continua reservada para gravacao, nao para estes controles.
where python >nul 2>nul
if errorlevel 1 goto sem_python
python -c "import serial" >nul 2>nul
if errorlevel 1 (
  python -m pip install -r "%~dp0tools\jrbot_frontend\requirements.txt"
  if errorlevel 1 goto falha
)
python "%~dp0tools\jrbot_frontend\app.py" --browser
if errorlevel 1 goto falha
exit /b 0
:sem_python
echo Python nao encontrado. Abra este arquivo pelo terminal ESP-IDF instalado.
goto falha
:falha
echo Nao foi possivel iniciar. Leia a mensagem acima. Nao abra outro painel em paralelo.
pause
exit /b 1
