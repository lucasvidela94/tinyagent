@echo off
setlocal EnableExtensions
rem Locale-independent timestamp (YYYYMMDD-HHMMSS); %DATE% depends on the Windows language.
set "LDT="
for /f "tokens=2 delims==" %%I in ('wmic os get LocalDateTime /value 2^>nul ^| find "="') do set "LDT=%%I"
if defined LDT (set "STAMP=%LDT:~0,8%-%LDT:~8,6%") else (set "STAMP=%RANDOM%%RANDOM%")
set "OUT=%~dp0diagnostics-%COMPUTERNAME%-%STAMP%"
if not exist "%OUT%" mkdir "%OUT%"
set "REPORT=%OUT%\report.txt"
(
  echo TinyShell - informe de diagnostico local
  echo Fecha: %DATE% %TIME%
  echo Equipo: %COMPUTERNAME%
  echo Usuario: %USERNAME%
  echo.
  echo ===== WINDOWS =====
  ver
  systeminfo
  echo.
  echo ===== CPU =====
  wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:list
  echo.
  echo ===== MEMORIA =====
  wmic OS get FreePhysicalMemory,TotalVisibleMemorySize /format:list
  echo.
  echo ===== DISCOS =====
  wmic logicaldisk get DeviceID,FileSystem,FreeSpace,Size /format:table
  echo.
  echo ===== ARRANQUE =====
  wmic os get Caption,Version,OSArchitecture,LastBootUpTime /format:list
  echo.
  echo ===== PROCESOS =====
  tasklist
  echo.
  echo ===== RED - PUEDE CONTENER DATOS SENSIBLES =====
  ipconfig /all
  echo.
  echo ===== SERVICIOS EN EJECUCION =====
  sc query type= service state= all
) > "%REPORT%" 2>&1

echo Informe creado en:
echo "%REPORT%"
echo.
echo REVISA Y REDACTA nombres de usuario, direcciones IP, MAC y otros datos antes de compartirlo.
pause
endlocal
