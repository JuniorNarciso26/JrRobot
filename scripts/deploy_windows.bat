@echo off
setlocal
cd /d "%~dp0\..\firmware\face_oled"
echo === JrBot Face OLED - build/flash/monitor ===
echo.
echo Uso: deploy_windows.bat COM4
echo Se nao passar porta, usa COM4.
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM4
where idf.py >nul 2>nul
if errorlevel 1 (
  echo ERRO: idf.py nao encontrado. Abra o terminal "ESP-IDF Command Prompt" e rode este script de novo.
  exit /b 1
)
echo Target: esp32s3
call idf.py set-target esp32s3 || exit /b 1
echo Build...
call idf.py build || exit /b 1
echo Gravando na porta %PORT%...
call idf.py -p %PORT% flash monitor
endlocal
