@echo off
setlocal
cd /d "%~dp0.."
where cl >nul 2>nul
if errorlevel 1 (
  echo No se encontro cl.exe. Ejecuta esto desde Visual Studio Developer Command Prompt.
  pause
  exit /b 1
)
cl /nologo /O1 /MT /W4 src\*.c winhttp.lib crypt32.lib advapi32.lib /Fe:tinyshell.exe
if errorlevel 1 exit /b 1
echo Compilado: tinyshell.exe
endlocal
