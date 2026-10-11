@echo off
rem TinyShell installer for Windows 7 and later. Run from a normal (not admin) CMD.
rem - Installs into %LOCALAPPDATA%\TinyShell and adds that folder to the user PATH.
rem - If tinyshell.exe sits next to this script (USB copy), installs it without downloading.
rem - Otherwise downloads the latest release with curl (Windows 10+) or bitsadmin (Windows 7).
rem Later updates: tinyshell --update (verifies the release signature).
setlocal EnableExtensions EnableDelayedExpansion
set "BASE=https://github.com/lucasvidela94/tinyagent/releases/latest/download"
if not defined LOCALAPPDATA set "LOCALAPPDATA=%USERPROFILE%\Local Settings\Application Data"
set "DIR=%LOCALAPPDATA%\TinyShell"
set "EXE=%DIR%\tinyshell.exe"
set "TMPEXE=%DIR%\tinyshell.download.exe"

if not exist "%DIR%" mkdir "%DIR%" || goto :fail
if exist "%TMPEXE%" del /q "%TMPEXE%"
rem goto instead of ( ) blocks: a path with ")" would break a parenthesized block.
if not exist "%~dp0tinyshell.exe" goto :fetch
echo Instalando la copia local: %~dp0tinyshell.exe
copy /y "%~dp0tinyshell.exe" "%TMPEXE%" >nul
goto :check
:fetch
echo Descargando TinyShell...
call :download "%BASE%/tinyshell.exe" "%TMPEXE%"
:check
if not exist "%TMPEXE%" goto :nodownload

"%TMPEXE%" --version >nul 2>&1 || goto :badexe
move /y "%TMPEXE%" "%EXE%" >nul || goto :fail
echo.
echo SHA-256 de tinyshell.exe (comparalo con la pagina del release):
certutil -hashfile "%EXE%" SHA256 | findstr /v ":"
call :addpath
echo.
"%EXE%" --version
echo Instalado en %DIR%
echo Abri una ventana NUEVA de CMD y escribi: tinyshell
echo Para actualizar mas adelante: tinyshell --update
exit /b 0

:download
where curl >nul 2>&1 && curl -fsSL -o "%~2" "%~1" && exit /b 0
if exist "%~2" del /q "%~2"
bitsadmin /transfer tinyshell /download /priority foreground "%~1" "%~2" >nul 2>&1
exit /b 0

:addpath
rem Only the user PATH (HKCU) is touched; setx PATH is avoided because it truncates
rem at 1024 characters and copies the system PATH into the user one.
set "UPATH="
for /f "skip=2 tokens=2,*" %%A in ('reg query HKCU\Environment /v Path 2^>nul') do set "UPATH=%%B"
echo ;!UPATH!; | find /i ";%DIR%;" >nul && goto :pathset
set "NEWPATH=%DIR%"
if defined UPATH set "NEWPATH=!UPATH!;%DIR%"
reg add HKCU\Environment /v Path /t REG_EXPAND_SZ /d "!NEWPATH!" /f >nul || echo No se pudo agregar %DIR% al PATH.
:pathset
rem setx broadcasts the environment change so new CMD windows pick up the PATH.
setx TINYSHELL_HOME "%DIR%" >nul
exit /b 0

:nodownload
echo.
echo No se pudo descargar TinyShell. En Windows 7 suele faltar TLS 1.2 para el sistema.
echo Alternativa: en otra PC baja tinyshell.exe e install.bat de
echo   https://github.com/lucasvidela94/tinyagent/releases/latest
echo copialos juntos en un USB y ejecuta install.bat desde ahi.
exit /b 1

:badexe
echo.
echo El ejecutable descargado no arranca en este equipo. No se instalo nada.
del /q "%TMPEXE%" 2>nul
exit /b 1

:fail
echo.
echo Fallo la instalacion en %DIR%.
exit /b 1
