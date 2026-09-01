@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "CRED_FILE=%~dp0credencial\wifi.txt"
set "ESP_IP=192.168.0.50"

if exist "%CRED_FILE%" (
  for /f "usebackq tokens=1,* delims==" %%A in ("%CRED_FILE%") do (
    if /i "%%A"=="IP_FIXO" set "ESP_IP=%%B"
  )
)

echo ========================================
echo JrBot - Painel Wi-Fi simples
echo ========================================
echo.
echo Abrindo painel direto no ESP32: http://%ESP_IP%
echo Se ainda estiver testando somente por Serial, pode fechar esta janela.
echo.
start "" "http://%ESP_IP%"
endlocal
