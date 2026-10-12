/* cmd.exe on Windows (XP to 11). */
#include "dialect.h"

static const RiskMark MARKS[] = {
    {"^", "usa ^, que puede ocultar el comando real"},
    {">", "redirige con > (crea o sobrescribe archivos)"},
    {"&", "encadena varios comandos con &"},
};

static const RiskRule RULES[] = {
    {"format", 0, "formatea discos"},
    {"diskpart", 0, "modifica discos o particiones"},
    {"convert", 0, "convierte el sistema de archivos"},
    {"del", 0, "borra archivos"},
    {"erase", 0, "borra archivos"},
    {"rd", 0, "borra carpetas"},
    {"rmdir", 0, "borra carpetas"},
    {"move", 0, "mueve archivos"},
    {"ren", 0, "renombra archivos"},
    {"rename", 0, "renombra archivos"},
    {"copy", 0, "copia archivos y puede sobrescribir"},
    {"xcopy", 0, "copia archivos y puede sobrescribir"},
    {"robocopy", 0, "copia o sincroniza archivos y puede borrar"},
    {"cipher", 0, "cifra o borra datos del disco"},
    {"fsutil", 0, "modifica el sistema de archivos"},
    {"chkdsk", "/f /r /x /b", "chkdsk con /f, /r, /x o /b escribe en el disco"},
    {"sfc", "/scannow /scanfile /scanonce /scanboot /revert", "repara archivos del sistema"},
    {"reg", "add delete import restore load unload copy", "modifica el registro"},
    {"regedit", 0, "modifica el registro"},
    {"sc", "config delete stop start create failure pause", "modifica servicios"},
    {"net", "stop start pause user localgroup share", "modifica servicios o cuentas"},
    {"netsh", "set add delete reset firewall advfirewall", "modifica la red o el firewall"},
    {"wmic", "delete call set create", "modifica el sistema vía WMI"},
    {"schtasks", "/create /delete /change /run", "modifica tareas programadas"},
    {"vssadmin", "delete resize", "borra puntos de restauración"},
    {"bcdedit", 0, "modifica el arranque"},
    {"bootrec", 0, "modifica el arranque"},
    {"bootsect", 0, "modifica el arranque"},
    {"shutdown", 0, "apaga o reinicia el equipo"},
    {"taskkill", 0, "termina procesos"},
    {"takeown", 0, "cambia permisos"},
    {"icacls", 0, "cambia permisos"},
    {"cacls", 0, "cambia permisos"},
    {"attrib", 0, "cambia atributos de archivos"},
    {"msiexec", 0, "instala o desinstala software"},
    {"wusa", 0, "instala o desinstala actualizaciones"},
    {"pnputil", 0, "modifica drivers"},
    {"powershell", 0, "ejecuta PowerShell"},
    {"wscript", 0, "ejecuta un script"},
    {"cscript", 0, "ejecuta un script"},
    {"mshta", 0, "ejecuta un script"},
    {"rundll32", 0, "ejecuta código de una DLL"},
    {"regsvr32", 0, "registra o ejecuta código de una DLL"},
};

const Dialect DIALECT_CMD = {
    MARKS, RULES, sizeof MARKS / sizeof MARKS[0], sizeof RULES / sizeof RULES[0],
    /* "DEL/S", "c:\windows\system32\format.com" and "\"rd\" x" all expose a bare word */
    " \t\"'@(),;=.:\\%|<", '/', 1, 0,
    "Windows",
    "Windows PC",
    "cmd.exe",
    "batch scripts",
    "no dir /s on a whole drive, no full event logs",
    "- Use built-in tools (cmd, wmic, reg query, sc, netsh, findstr, type, where). PowerShell may be missing or "
    "very old on these systems; do not assume it, and do not assume modern command options exist.\n",
    "deleting, registry, services, drivers, partitions, installing, security settings",
};
