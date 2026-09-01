@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Scanner I2C ESP32-S3
 echo ========================================
echo.
echo Uso: INSTALAR_SCANNER_I2C_WINDOWS.bat COM6
echo Se nao informar a porta, usa COM4.
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM4
where idf.py >nul 2>nul
if errorlevel 1 (
  echo ERRO: idf.py nao encontrado. Abra pelo ESP-IDF Command Prompt.
  pause
  exit /b 1
)
cd /d "%~dp0\firmware\i2c_scanner"
call idf.py set-target esp32s3 || goto erro
call idf.py build || goto erro
call idf.py -p %PORT% flash monitor || goto erro
goto fim
:erro
echo Falhou. Me mande o log.
pause
exit /b 1
:fim
endlocal
