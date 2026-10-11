@echo off
setlocal
cd /d "%~dp0.."
where cl >nul 2>nul
if errorlevel 1 (
  echo No se encontro cl.exe. Ejecuta esto desde Visual Studio Developer Command Prompt.
  pause
  exit /b 1
)
rem Layout: see build/sources.sh
cl /nologo /O1 /MT /W4 /Isrc /Isrc\core /Isrc\app /Isrc\platform /Isrc\platform\win32 src\core\*.c src\app\*.c src\platform\win32\*.c winhttp.lib crypt32.lib advapi32.lib /Fe:tinyshell.exe
if errorlevel 1 exit /b 1
echo Compilado: tinyshell.exe
endlocal
