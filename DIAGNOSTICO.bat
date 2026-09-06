@echo off
rem Diagnostics are in the existing panel, not a separate Serial monitor.
call "%~dp0PAINEL.bat"
exit /b %errorlevel%
