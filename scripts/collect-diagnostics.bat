@echo off
setlocal EnableExtensions
set "OUT=%~dp0diagnostics-%COMPUTERNAME%"
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
