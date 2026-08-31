@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot Face OLED - Instalador ESP32-S3
 echo ========================================
echo.
echo Este instalador compila e grava o firmware na ESP32-S3.
echo Requisito: abrir pelo "ESP-IDF Command Prompt" no Windows.
echo.
echo Uso:
echo   INSTALAR_JRBOT_WINDOWS.bat COM4
echo.
echo Se nao informar a porta, usa COM4.
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM4
where idf.py >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERRO: idf.py nao encontrado.
  echo Abra o terminal "ESP-IDF Command Prompt" e execute este arquivo novamente.
  echo.
  pause
  exit /b 1
)
echo Porta: %PORT%
echo Target: esp32s3
echo.
cd /d "%~dp0\firmware\face_oled"
call idf.py set-target esp32s3 || goto erro
call idf.py build || goto erro
call idf.py -p %PORT% flash monitor || goto erro
goto fim
:erro
echo.
echo Falhou. Me mande a tela/log do erro para eu corrigir.
pause
exit /b 1
:fim
endlocal
