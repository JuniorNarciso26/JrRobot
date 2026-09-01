@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Instalador ESP32-S3
 echo ========================================
echo.
echo Este instalador compila e grava o firmware na ESP32-S3.
echo No final ele abre automaticamente o painel HTML local.
echo Requisito: abrir pelo "ESP-IDF Command Prompt" no Windows.
echo.
echo Uso:
echo   INSTALAR_JRBOT_WINDOWS.bat COM6
echo.
echo Se nao informar a porta, usa COM6.
echo Dica: nesta placa a gravacao pode ser COM6 e o painel/log pode ser COM4.
echo.
set PORT=%1
if "%PORT%"=="" set PORT=COM6
where idf.py >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERRO: idf.py nao encontrado.
  echo Abra o terminal "ESP-IDF Command Prompt" e execute este arquivo novamente.
  echo.
  pause
  exit /b 1
)
echo Porta de gravacao: %PORT%
echo Target: esp32s3
echo.
cd /d "%~dp0\firmware\esp32"
call idf.py set-target esp32s3 || goto erro
call idf.py build || goto erro
call idf.py -p %PORT% flash || goto erro
cd /d "%~dp0"
echo.
echo Firmware gravado com sucesso.
echo Abrindo painel HTML local...
echo Se o Windows perguntar, permita o Python na rede local apenas se quiser usar o painel.
start "" "%~dp0\tools\jrbot_frontend\index.html"
start "JrBot Painel Local" cmd /k "%~dp0\ABRIR_PAINEL_JRBOT_WINDOWS.bat"
goto fim
:erro
echo.
echo Falhou. Me mande a tela/log do erro para eu corrigir.
pause
exit /b 1
:fim
endlocal
