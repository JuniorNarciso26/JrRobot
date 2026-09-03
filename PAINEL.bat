@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
echo ========================================
echo JrBot - Painel Unico
echo ========================================
echo.
echo Abrindo uma unica tela para Serial USB e Wi-Fi.
echo Endereco local: http://127.0.0.1:8765
echo.
echo Na tela voce escolhe:
echo   - Serial USB para configurar/testar/diagnosticar
echo   - Wi-Fi usando o IP que apareceu no log, exemplo 192.168.0.83
echo.
where python >nul 2>nul
if errorlevel 1 (
  echo ERRO: Python nao encontrado no PATH.
  echo Instale Python 3 ou abra por um terminal onde o comando python funcione.
  pause
  exit /b 1
)
python -c "import serial" >nul 2>nul
if errorlevel 1 (
  echo Instalando dependencia pyserial...
  python -m pip install -r "%~dp0tools\jrbot_frontend\requirements.txt" || goto erro_python
)
start "" "http://127.0.0.1:8765"
python "%~dp0tools\jrbot_frontend\app.py"
goto fim

:erro_python
echo.
echo Falhou ao instalar pyserial.
echo Tente manualmente: python -m pip install pyserial
pause
exit /b 1

:fim
endlocal
