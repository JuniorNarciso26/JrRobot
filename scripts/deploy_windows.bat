@echo off
setlocal
if not "%~1"=="" set "JR_FLASH_PORT=%~1"
call "%~dp0..\DIAG_V2.bat" flash
exit /b %errorlevel%
