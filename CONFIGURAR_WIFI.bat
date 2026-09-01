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
  >> "%CRED_FILE%" echo.
  >> "%CRED_FILE%" echo # IP fixo opcional. Use ATIVAR_IP_FIXO=1 para ligar, ou 0 para usar DHCP automatico.
  >> "%CRED_FILE%" echo ATIVAR_IP_FIXO=1
  >> "%CRED_FILE%" echo IP_FIXO=192.168.0.50
  >> "%CRED_FILE%" echo GATEWAY=192.168.0.1
  >> "%CRED_FILE%" echo MASCARA=255.255.255.0
  >> "%CRED_FILE%" echo DNS1=8.8.8.8
  >> "%CRED_FILE%" echo DNS2=8.8.4.4
)

set "WIFI_SSID="
set "WIFI_PASS="
set "WIFI_HOST=jrbot"
set "USE_STATIC_IP=1"
set "STATIC_IP=192.168.0.50"
set "GATEWAY=192.168.0.1"
set "SUBNET=255.255.255.0"
set "DNS1=8.8.8.8"
set "DNS2=8.8.4.4"

for /f "usebackq tokens=1,* delims==" %%A in ("%CRED_FILE%") do (
  set "K=%%A"
  set "V=%%B"
  if /i "!K!"=="NOME_WIFI" set "WIFI_SSID=!V!"
  if /i "!K!"=="SENHA_WIFI" set "WIFI_PASS=!V!"
  if /i "!K!"=="NOME_DO_DISPOSITIVO" set "WIFI_HOST=!V!"
  if /i "!K!"=="ATIVAR_IP_FIXO" set "USE_STATIC_IP=!V!"
  if /i "!K!"=="IP_FIXO" set "STATIC_IP=!V!"
  if /i "!K!"=="GATEWAY" set "GATEWAY=!V!"
  if /i "!K!"=="MASCARA" set "SUBNET=!V!"
  if /i "!K!"=="DNS1" set "DNS1=!V!"
  if /i "!K!"=="DNS2" set "DNS2=!V!"
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
  echo #define JR_WIFI_USE_STATIC_IP %USE_STATIC_IP%
  echo #define JR_WIFI_STATIC_IP "%STATIC_IP%"
  echo #define JR_WIFI_GATEWAY "%GATEWAY%"
  echo #define JR_WIFI_SUBNET "%SUBNET%"
  echo #define JR_WIFI_DNS1 "%DNS1%"
  echo #define JR_WIFI_DNS2 "%DNS2%"
) > "%~dp0firmware\main\wifi_config.local.h"

echo.
echo Wi-Fi configurado localmente usando: %CRED_FILE%
echo IP fixo: %USE_STATIC_IP% - %STATIC_IP%
echo Agora rode: .\INSTALAR.bat COM6
pause
endlocal
