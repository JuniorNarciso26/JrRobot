@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Instalador ESP32-S3
echo ========================================
echo.
echo Este instalador compila e grava o firmware na ESP32-S3.
echo Para testar via Wi-Fi, rode CONFIGURAR_WIFI.bat antes deste instalador.
echo.
echo Uso:
echo   .\INSTALAR.bat COM6
echo.
echo Se nao informar a porta, usa COM6.
echo Dica: nesta placa a gravacao pode ser COM6 e o painel/log pode ser COM4.
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM6

where idf.py >nul 2>nul
if errorlevel 1 call :carregar_espidf

where idf.py >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERRO: idf.py nao encontrado.
  echo.
  echo Como resolver:
  echo 1. Abra pelo menu iniciar: ESP-IDF 5.5 CMD ou ESP-IDF Command Prompt
  echo 2. Entre na pasta do JrBot
  echo 3. Rode: .\INSTALAR.bat %PORT%
  echo.
  echo Tambem tentei carregar automaticamente em C:\Espressif, mas nao encontrei o ambiente.
  echo.
  pause
  exit /b 1
)

echo Porta de gravacao: %PORT%
echo Target: esp32s3
echo.
cd /d "%~dp0firmware"
call idf.py set-target esp32s3 || goto erro
call idf.py build || goto erro
call idf.py -p %PORT% flash || goto erro
cd /d "%~dp0"
echo.
echo Firmware gravado com sucesso.
echo Se o Wi-Fi foi configurado, veja no log do ESP32 a linha JR_WIFI com o IP.
echo Para testar por Serial, abra o monitor pela porta de log.
goto fim

:carregar_espidf
echo idf.py nao esta no PATH. Tentando carregar ESP-IDF automaticamente...
set "IDF_TOOLS_PATH=C:\Espressif"
if exist "C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat" call "C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat"
if exist "C:\Espressif\frameworks\esp-idf-v5.5\export.bat" call "C:\Espressif\frameworks\esp-idf-v5.5\export.bat"
if exist "C:\Espressif\frameworks\esp-idf-v5.4\export.bat" call "C:\Espressif\frameworks\esp-idf-v5.4\export.bat"
exit /b 0

:erro
echo.
echo Falhou. Me mande a tela/log do erro para eu corrigir.
pause
exit /b 1

:fim
endlocal
