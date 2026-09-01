@echo off
setlocal
cd /d "%~dp0\firmware"
echo Capturando log de build em: %~dp0erro-build.txt
idf.py build > "%~dp0erro-build.txt" 2>&1
cd /d "%~dp0"
echo.
echo Log salvo em erro-build.txt
echo Me mande esse arquivo se falhar.
pause
endlocal
