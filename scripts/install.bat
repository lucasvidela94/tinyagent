@echo off
rem TinyShell: downloads the installer (TinyShell-Setup.exe) and runs it. Kept for the
rem one-line commands and links published before the installer existed; the installer
rem itself is the recommended download. Run from a normal (not admin) CMD.
rem If TinyShell-Setup.exe sits next to this script (USB copy), it runs that one instead.
setlocal EnableExtensions
set "URL=https://github.com/lucasvidela94/tinyagent/releases/latest/download/TinyShell-Setup.exe"
set "SETUP=%TEMP%\TinyShell-Setup.exe"
set "ES="
set "LOC=en"
for /f "skip=2 tokens=3" %%L in ('reg query "HKCU\Control Panel\International" /v LocaleName 2^>nul') do set "LOC=%%L"
if /i "%LOC:~0,2%"=="es" set "ES=1"

if not exist "%~dp0TinyShell-Setup.exe" goto :fetch
set "SETUP=%~dp0TinyShell-Setup.exe"
goto :run
:fetch
if defined ES (echo Descargando el instalador de TinyShell...) else (echo Downloading the TinyShell installer...)
if exist "%SETUP%" del /q "%SETUP%"
where curl >nul 2>&1 && curl -fsSL -o "%SETUP%" "%URL%"
if not exist "%SETUP%" bitsadmin /transfer tinyshell /download /priority foreground "%URL%" "%SETUP%" >nul 2>&1
if not exist "%SETUP%" goto :nodownload
:run
if defined ES (echo SHA-256 del instalador, comparalo con la pagina del release:) else (echo Installer SHA-256, compare it with the release page:)
certutil -hashfile "%SETUP%" SHA256 | findstr /v ":"
"%SETUP%"
exit /b %errorlevel%

:nodownload
echo.
if defined ES goto :nodownload_es
echo Could not download the installer. Windows 7 often lacks system-wide TLS 1.2.
echo Instead: on another PC download TinyShell-Setup.exe from
echo   https://github.com/lucasvidela94/tinyagent/releases/latest
echo copy it to a USB stick and run it on this PC.
exit /b 1
:nodownload_es
echo No se pudo descargar el instalador. En Windows 7 suele faltar TLS 1.2 para el sistema.
echo Alternativa: en otra PC baja TinyShell-Setup.exe de
echo   https://github.com/lucasvidela94/tinyagent/releases/latest
echo copialo a un pendrive y ejecutalo en esta PC.
exit /b 1
