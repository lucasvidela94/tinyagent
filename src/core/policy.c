#include "policy.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

const char *command_problem(const ToolCall *tc) {
  const unsigned char *p;
  if (strcmp(tc->name, "shell")) return "Herramienta no permitida. No se ejecutó nada.";
  if (!*tc->command) return "Faltó el comando (argumentos ausentes o JSON inválido). No se ejecutó nada.";
  if (strlen(tc->command) > MAX_COMMAND) return "Comando demasiado largo. No se ejecutó nada.";
  for (p = (const unsigned char *)tc->command; *p; p++)
    if (*p < 0x20 || *p == 0x7F) return "El comando tiene saltos de línea o caracteres de control. No se ejecutó nada.";
  return NULL;
}

/* A rule fires when `word` appears and, if `flags` is set, any one of its space-separated flags too. */
static const struct { const char *word, *flags, *reason; } RULES[] = {
    {"format", NULL, "formatea discos"},
    {"diskpart", NULL, "modifica discos o particiones"},
    {"convert", NULL, "convierte el sistema de archivos"},
    {"del", NULL, "borra archivos"},
    {"erase", NULL, "borra archivos"},
    {"rd", NULL, "borra carpetas"},
    {"rmdir", NULL, "borra carpetas"},
    {"move", NULL, "mueve archivos"},
    {"ren", NULL, "renombra archivos"},
    {"rename", NULL, "renombra archivos"},
    {"copy", NULL, "copia archivos y puede sobrescribir"},
    {"xcopy", NULL, "copia archivos y puede sobrescribir"},
    {"robocopy", NULL, "copia o sincroniza archivos y puede borrar"},
    {"cipher", NULL, "cifra o borra datos del disco"},
    {"fsutil", NULL, "modifica el sistema de archivos"},
    {"chkdsk", "/f /r /x /b", "chkdsk con /f, /r, /x o /b escribe en el disco"},
    {"sfc", "/scannow /scanfile /scanonce /scanboot /revert", "repara archivos del sistema"},
    {"reg", "add delete import restore load unload copy", "modifica el registro"},
    {"regedit", NULL, "modifica el registro"},
    {"sc", "config delete stop start create failure pause", "modifica servicios"},
    {"net", "stop start pause user localgroup share", "modifica servicios o cuentas"},
    {"netsh", "set add delete reset firewall advfirewall", "modifica la red o el firewall"},
    {"wmic", "delete call set create", "modifica el sistema vía WMI"},
    {"schtasks", "/create /delete /change /run", "modifica tareas programadas"},
    {"vssadmin", "delete resize", "borra puntos de restauración"},
    {"bcdedit", NULL, "modifica el arranque"},
    {"bootrec", NULL, "modifica el arranque"},
    {"bootsect", NULL, "modifica el arranque"},
    {"shutdown", NULL, "apaga o reinicia el equipo"},
    {"taskkill", NULL, "termina procesos"},
    {"takeown", NULL, "cambia permisos"},
    {"icacls", NULL, "cambia permisos"},
    {"cacls", NULL, "cambia permisos"},
    {"attrib", NULL, "cambia atributos de archivos"},
    {"msiexec", NULL, "instala o desinstala software"},
    {"wusa", NULL, "instala o desinstala actualizaciones"},
    {"pnputil", NULL, "modifica drivers"},
    {"powershell", NULL, "ejecuta PowerShell"},
    {"wscript", NULL, "ejecuta un script"},
    {"cscript", NULL, "ejecuta un script"},
    {"mshta", NULL, "ejecuta un script"},
    {"rundll32", NULL, "ejecuta código de una DLL"},
    {"regsvr32", NULL, "registra o ejecuta código de una DLL"},
};

static int has_word(const char *text, const char *w, size_t n) { /* text is " word word " */
  char pat[40];
  if (n + 3 > sizeof pat) return 0;
  pat[0] = ' ';
  memcpy(pat + 1, w, n);
  pat[n + 1] = ' ';
  pat[n + 2] = 0;
  return strstr(text, pat) != NULL;
}

static int rule_fires(const char *text, size_t r) {
  const char *f = RULES[r].flags;
  if (!has_word(text, RULES[r].word, strlen(RULES[r].word))) return 0;
  if (!f) return 1;
  while (*f) {
    size_t n = strcspn(f, " ");
    if (has_word(text, f, n)) return 1;
    f += n;
    while (*f == ' ') f++;
  }
  return 0;
}

const char *risk_reason(const char *cmd) {
  Buf b = {0};
  const char *p, *found = NULL;
  size_t r;
  if (strchr(cmd, '^')) return "usa ^, que puede ocultar el comando real";
  if (strchr(cmd, '>')) return "redirige con > (crea o sobrescribe archivos)";
  if (strchr(cmd, '&')) return "encadena varios comandos con &";
  /* Lowercase and split on cmd delimiters and path/extension separators, so "DEL/S",
   * "c:\windows\system32\format.com" and "\"rd\" x" all expose a bare word. */
  buf_str(&b, " ");
  for (p = cmd; *p; p++) {
    char c = (char)tolower((unsigned char)*p);
    if (c == '/') buf_str(&b, " /");
    else if (strchr(" \t\"'@(),;=.:\\%|<", c)) buf_str(&b, " ");
    else buf_add(&b, &c, 1);
  }
  buf_str(&b, " ");
  for (r = 0; r < sizeof RULES / sizeof RULES[0] && !found; r++)
    if (rule_fires(b.p, r)) found = RULES[r].reason;
  free(b.p);
  return found;
}

int is_yes(const char *answer) {
  char a[8];
  size_t n = 0;
  while (*answer == ' ' || *answer == '\t') answer++;
  while (*answer && n < sizeof a - 1) a[n++] = (char)tolower((unsigned char)*answer++);
  a[n] = 0;
  trim_line(a);
  if (*answer) return 0; /* longer than any accepted answer */
  return !strcmp(a, "s") || !strcmp(a, "si") || !strcmp(a, "s\xc3\xad") || !strcmp(a, "s\xc3\x8d");
}
