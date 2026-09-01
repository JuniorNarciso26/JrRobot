@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot Face OLED - Gravar sem monitor serial
echo ========================================
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM6
where idf.py >nul 2>nul
if errorlevel 1 (
  echo ERRO: idf.py nao encontrado. Abra pelo ESP-IDF Command Prompt.
  pause
  exit /b 1
)
cd /d "%~dp0\firmware\face_oled"
call idf.py set-target esp32s3 || goto erro
call idf.py build || goto erro
call idf.py -p %PORT% flash || goto erro
echo.
echo Gravado com sucesso. Agora volte para a pasta jrbot e rode:
echo .\CONSOLE_SERIAL_JRBOT_WINDOWS.bat %PORT%
pause
exit /b 0
:erro
echo Falhou. Me mande o log.
pause
exit /b 1
endlocal
