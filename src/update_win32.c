#include "win32.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "policy.h"
#include "shell.h"
#include "update.h"
#include "util.h"
#include "verify.h"
#include "version.h"

typedef struct { wchar_t self[MAX_PATH], fresh[MAX_PATH + 8], old[MAX_PATH + 8]; } Paths;

/* <dir>\tinyshell.exe -> <dir>\tinyshell.new.exe and <dir>\tinyshell.old.exe (same folder,
 * so renames stay on one volume; ".exe" kept so cmd.exe can run the new one). */
static int exe_paths(Paths *p) {
  DWORD n = GetModuleFileNameW(NULL, p->self, MAX_PATH);
  if (!n || n >= MAX_PATH || n < 4 || lstrcmpiW(p->self + n - 4, L".exe")) return 0;
  lstrcpyW(p->fresh, p->self);
  lstrcpyW(p->fresh + n - 4, L".new.exe");
  lstrcpyW(p->old, p->self);
  lstrcpyW(p->old + n - 4, L".old.exe");
  return 1;
}

static char *utf8(const wchar_t *w) {
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  char *s = (char *)xrealloc(NULL, (size_t)(n > 0 ? n : 1));
  if (n <= 0 || !WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL)) s[0] = 0;
  return s;
}

static int write_file(const wchar_t *path, const char *data, size_t n) {
  DWORD done = 0;
  HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  int ok;
  if (f == INVALID_HANDLE_VALUE) return 0;
  ok = WriteFile(f, data, (DWORD)n, &done, NULL) && done == n;
  return CloseHandle(f) && ok;
}

/* Runs `"<fresh>" --version` and reads the version it reports. Proves the new binary loads
 * on this Windows (DLLs, CRT) and reports a version that is covered by the signature. */
static int self_test(const wchar_t *fresh, char *ver, size_t cap) {
  char *path = utf8(fresh), *out;
  Buf cmd = {0};
  int ok;
  buf_str(&cmd, "\"");
  buf_str(&cmd, path);
  buf_str(&cmd, "\" --version");
  free(path);
  out = run_command(cmd.p);
  ok = version_from_output(out, ver, cap) && strstr(out, "[exit=0]") != NULL;
  free(out);
  free(cmd.p);
  return ok;
}

static int fetch_latest(char *latest, size_t cap) {
  size_t len = 0;
  char *v = http_get(UPDATE_BASE_URL "version.txt", 64, &len);
  if (!v) return 0;
  trim_line(v);
  if (strlen(v) + 1 > cap || version_compare(v, TINYSHELL_VERSION) == VERSION_INVALID) {
    say("version.txt del release es inválido.\n");
    free(v);
    return 0;
  }
  strcpy(latest, v);
  free(v);
  return 1;
}

/* Downloads, verifies and stages the new binary at p->fresh. 1 if ready to swap. */
static int stage(const Paths *p, const char *latest) {
  size_t exe_len = 0, sig_len = 0;
  char *exe = http_get(UPDATE_BASE_URL "tinyshell.exe", MAX_UPDATE, &exe_len);
  char *sig = exe ? http_get(UPDATE_BASE_URL "tinyshell.exe.sig", 2048, &sig_len) : NULL;
  char hex[65], tested[32];
  int ok = 0;
  if (!exe || !sig) goto done;
  sha256_hex((const unsigned char *)exe, exe_len, hex);
  say("Descargado: %lu bytes, SHA-256 %s\n", (unsigned long)exe_len, hex);
  if (!signature_valid((const unsigned char *)exe, exe_len, (const unsigned char *)sig, sig_len)) {
    say("FIRMA INVÁLIDA: el binario no viene de la clave de releases. No se instaló nada.\n");
    goto done;
  }
  say("Firma verificada.\n");
  if (!write_file(p->fresh, exe, exe_len)) {
    say("No se pudo escribir el binario nuevo (¿carpeta sin permiso de escritura?).\n");
    goto done;
  }
  if (!self_test(p->fresh, tested, sizeof tested) || version_compare(tested, latest) != 0 ||
      version_compare(tested, TINYSHELL_VERSION) <= 0) {
    say("El binario nuevo no pasó la prueba de arranque en este equipo. No se instaló nada.\n");
    DeleteFileW(p->fresh);
    goto done;
  }
  ok = 1;
done:
  free(exe);
  free(sig);
  return ok;
}

int update_run(void) {
  Paths p;
  char latest[32], answer[64];
  if (!release_key_present()) {
    say("Este binario no trae la clave pública de releases: no puede verificar actualizaciones.\n");
    return 1;
  }
  if (!exe_paths(&p)) {
    say("No se pudo ubicar el ejecutable actual.\n");
    return 1;
  }
  say("TinyShell %s. Buscando actualizaciones...\n", TINYSHELL_VERSION);
  if (!fetch_latest(latest, sizeof latest)) return 1;
  if (version_compare(latest, TINYSHELL_VERSION) <= 0) {
    say("Ya tenés la última versión.\n");
    return 0;
  }
  say("Hay una versión nueva: %s. ¿Instalarla? [s/N]: ", latest);
  if (read_line(answer, sizeof answer, 1) != LINE_OK || !is_yes(answer)) {
    say("Cancelado.\n");
    return 1;
  }
  if (!stage(&p, latest)) return 1;
  /* A running .exe cannot be overwritten, but it can be renamed. */
  if (!MoveFileExW(p.self, p.old, MOVEFILE_REPLACE_EXISTING)) {
    say("No se pudo apartar el binario actual (error %lu). No se instaló nada.\n", (unsigned long)GetLastError());
    DeleteFileW(p.fresh);
    return 1;
  }
  if (!MoveFileExW(p.fresh, p.self, 0)) {
    say("No se pudo instalar el binario nuevo (error %lu); se restauró el anterior.\n", (unsigned long)GetLastError());
    MoveFileExW(p.old, p.self, 0);
    DeleteFileW(p.fresh);
    return 1;
  }
  say("Actualizado a %s. Cerrá y volvé a abrir TinyShell.\nSi algo falla: tinyshell --rollback\n", latest);
  return 0;
}

int update_rollback(void) {
  Paths p;
  if (!exe_paths(&p)) {
    say("No se pudo ubicar el ejecutable actual.\n");
    return 1;
  }
  if (GetFileAttributesW(p.old) == INVALID_FILE_ATTRIBUTES) {
    say("No hay versión anterior guardada (tinyshell.old.exe).\n");
    return 1;
  }
  /* self -> fresh (temporary), old -> self, fresh -> old */
  if (!MoveFileExW(p.self, p.fresh, MOVEFILE_REPLACE_EXISTING)) {
    say("No se pudo apartar el binario actual (error %lu).\n", (unsigned long)GetLastError());
    return 1;
  }
  if (!MoveFileExW(p.old, p.self, 0)) {
    say("No se pudo restaurar la versión anterior (error %lu).\n", (unsigned long)GetLastError());
    MoveFileExW(p.fresh, p.self, 0);
    return 1;
  }
  MoveFileExW(p.fresh, p.old, MOVEFILE_REPLACE_EXISTING);
  say("Versión anterior restaurada. Cerrá y volvé a abrir TinyShell.\n");
  return 0;
}
