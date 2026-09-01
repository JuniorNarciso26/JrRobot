@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "CRED_DIR=%~dp0credencial"
set "CRED_FILE=%CRED_DIR%\wifi.txt"

echo ========================================
echo JrBot - Configurar Wi-Fi
echo ========================================
echo.

if not exist "%CRED_DIR%" mkdir "%CRED_DIR%"

if not exist "%CRED_FILE%" (
  echo Criando arquivo de credencial local...
  > "%CRED_FILE%" echo # JrBot - credencial local de Wi-Fi
  >> "%CRED_FILE%" echo # Este arquivo fica na maquina do Junior e NAO deve ir para o GitHub.
  >> "%CRED_FILE%" echo NOME_WIFI=COLOQUE_AQUI_O_NOME_DA_REDE
  >> "%CRED_FILE%" echo SENHA_WIFI=COLOQUE_AQUI_A_SENHA_DA_REDE
  >> "%CRED_FILE%" echo NOME_DO_DISPOSITIVO=jrbot
)

set "WIFI_SSID="
set "WIFI_PASS="
set "WIFI_HOST=jrbot"

for /f "usebackq tokens=1,* delims==" %%A in ("%CRED_FILE%") do (
  set "K=%%A"
  set "V=%%B"
  if /i "!K!"=="NOME_WIFI" set "WIFI_SSID=!V!"
  if /i "!K!"=="SENHA_WIFI" set "WIFI_PASS=!V!"
  if /i "!K!"=="NOME_DO_DISPOSITIVO" set "WIFI_HOST=!V!"
)

if "%WIFI_SSID%"=="" set /p WIFI_SSID=Nome da rede Wi-Fi: 
if "%WIFI_SSID%"=="COLOQUE_AQUI_O_NOME_DA_REDE" set /p WIFI_SSID=Nome da rede Wi-Fi: 
if "%WIFI_PASS%"=="" set /p WIFI_PASS=Senha do Wi-Fi: 
if "%WIFI_PASS%"=="COLOQUE_AQUI_A_SENHA_DA_REDE" set /p WIFI_PASS=Senha do Wi-Fi: 
if "%WIFI_HOST%"=="" set "WIFI_HOST=jrbot"

if "%WIFI_SSID%"=="" (
  echo Nome da rede vazio.
  pause
  exit /b 1
)

(
  echo #pragma once
  echo #define JR_WIFI_SSID "%WIFI_SSID%"
  echo #define JR_WIFI_PASSWORD "%WIFI_PASS%"
  echo #define JR_WIFI_HOSTNAME "%WIFI_HOST%"
) > "%~dp0firmware\esp32\main\wifi_config.local.h"

echo.
echo Wi-Fi configurado localmente usando: %CRED_FILE%
echo Agora rode: .\INSTALAR.bat COM6
pause
endlocal
