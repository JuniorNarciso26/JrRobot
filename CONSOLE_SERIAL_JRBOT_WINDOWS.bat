@echo off
setlocal
cd /d "%~dp0\tools\jrbot_frontend"
set PORT=%1
if "%PORT%"=="" set PORT=COM6
where python >nul 2>nul
if errorlevel 1 (
  echo ERRO: Python nao encontrado.
  pause
  exit /b 1
)
python -m pip install pyserial
python serial_console.py %PORT%
pause
endlocal
