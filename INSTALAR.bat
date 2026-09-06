@echo off
rem Entry point retained for existing users; never use an old build directory.
call "%~dp0DIAG_V2.bat" flash
exit /b %errorlevel%
