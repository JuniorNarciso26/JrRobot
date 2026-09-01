@echo off
setlocal
cd /d "%~dp0"
echo ========================================
echo JrBot - Configurar Wi-Fi
echo ========================================
echo.
set /p WIFI_SSID=Nome da rede Wi-Fi: 
set /p WIFI_PASS=Senha do Wi-Fi: 
if "%WIFI_SSID%"=="" (
  echo Nome da rede vazio.
  pause
  exit /b 1
)
(
  echo #pragma once
  echo #define JR_WIFI_SSID "%WIFI_SSID%"
  echo #define JR_WIFI_PASSWORD "%WIFI_PASS%"
  echo #define JR_WIFI_HOSTNAME "jrbot"
) > "%~dp0firmware\esp32\main\wifi_config.local.h"
echo.
echo Wi-Fi configurado localmente.
echo Agora rode: .\INSTALAR.bat COM6
pause
endlocal
