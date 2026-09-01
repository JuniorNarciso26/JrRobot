@echo off
setlocal
cd /d "%~dp0\tools\jrbot_frontend"
where python >nul 2>nul
if errorlevel 1 (
  echo ERRO: Python nao encontrado no Windows.
  echo Instale Python 3 ou abra pelo terminal que tenha Python.
  pause
  exit /b 1
)
python -m pip install -r requirements.txt
start "JrBot Servidor Local" cmd /k python app.py
timeout /t 2 /nobreak >nul
start "" http://127.0.0.1:8765
endlocal
