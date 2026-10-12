/* cmd.exe on Windows (XP to 11). */
#include "dialect.h"
#include "i18n.h"

static const RiskMark MARKS[] = {
    {"^", N_("uses ^, which can hide the real command")},
    {">", N_("redirects with > (creates or overwrites files)")},
    {"&", N_("chains several commands with &")},
};

static const RiskRule RULES[] = {
    {"format", 0, N_("formats disks")},
    {"diskpart", 0, N_("changes disks or partitions")},
    {"convert", 0, N_("converts the file system")},
    {"del", 0, N_("deletes files")},
    {"erase", 0, N_("deletes files")},
    {"rd", 0, N_("deletes folders")},
    {"rmdir", 0, N_("deletes folders")},
    {"move", 0, N_("moves files")},
    {"ren", 0, N_("renames files")},
    {"rename", 0, N_("renames files")},
    {"copy", 0, N_("copies files and may overwrite them")},
    {"xcopy", 0, N_("copies files and may overwrite them")},
    {"robocopy", 0, N_("copies or syncs files and may delete")},
    {"cipher", 0, N_("encrypts or wipes disk data")},
    {"fsutil", 0, N_("changes the file system")},
    {"chkdsk", "/f /r /x /b", N_("chkdsk with /f, /r, /x or /b writes to the disk")},
    {"sfc", "/scannow /scanfile /scanonce /scanboot /revert", N_("repairs system files")},
    {"reg", "add delete import restore load unload copy", N_("changes the registry")},
    {"regedit", 0, N_("changes the registry")},
    {"sc", "config delete stop start create failure pause", N_("changes services")},
    {"net", "stop start pause user localgroup share", N_("changes services or accounts")},
    {"netsh", "set add delete reset firewall advfirewall", N_("changes the network or the firewall")},
    {"wmic", "delete call set create", N_("changes the system through WMI")},
    {"schtasks", "/create /delete /change /run", N_("changes scheduled tasks")},
    {"vssadmin", "delete resize", N_("deletes restore points")},
    {"bcdedit", 0, N_("changes the boot setup")},
    {"bootrec", 0, N_("changes the boot setup")},
    {"bootsect", 0, N_("changes the boot setup")},
    {"shutdown", 0, N_("shuts down or restarts the computer")},
    {"taskkill", 0, N_("ends processes")},
    {"takeown", 0, N_("changes permissions")},
    {"icacls", 0, N_("changes permissions")},
    {"cacls", 0, N_("changes permissions")},
    {"attrib", 0, N_("changes file attributes")},
    {"msiexec", 0, N_("installs or uninstalls software")},
    {"wusa", 0, N_("installs or uninstalls updates")},
    {"pnputil", 0, N_("changes drivers")},
    {"powershell", 0, N_("runs PowerShell")},
    {"wscript", 0, N_("runs a script")},
    {"cscript", 0, N_("runs a script")},
    {"mshta", 0, N_("runs a script")},
    {"rundll32", 0, N_("runs code from a DLL")},
    {"regsvr32", 0, N_("registers or runs code from a DLL")},
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
