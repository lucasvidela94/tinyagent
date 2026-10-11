#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "policy.h"
#include "shell.h"
#include "sys.h"
#include "update.h"
#include "util.h"
#include "verify.h"
#include "version.h"

/* The running binary and its two siblings: fresh (the download being installed) and old
 * (the previous version, for --rollback). */
typedef struct { Buf self, fresh, old; } Paths;

static int paths_init(Paths *p) {
  memset(p, 0, sizeof *p);
  if (!sys_self_path(&p->self)) return 0;
  version_sibling(p->self.p, "new", &p->fresh);
  version_sibling(p->self.p, "old", &p->old);
  return 1;
}

static void paths_free(Paths *p) {
  free(p->self.p);
  free(p->fresh.p);
  free(p->old.p);
}

/* Runs `"<fresh>" --version` and reads the version it reports. Proves the new binary loads
 * on this system (libraries, CRT) and reports a version that is covered by the signature. */
static int self_test(const char *fresh, char *ver, size_t cap) {
  Buf cmd = {0};
  char *out;
  int ok;
  buf_str(&cmd, "\"");
  buf_str(&cmd, fresh);
  buf_str(&cmd, "\" --version");
  out = run_command(cmd.p);
  ok = version_from_output(out, ver, cap) && strstr(out, "[exit=0]") != NULL;
  free(out);
  free(cmd.p);
  return ok;
}

static int fetch_latest(char *latest, size_t cap) {
  size_t len = 0;
  char *v = http_get(UPDATE_BASE_URL "version.txt", NULL, 64, &len);
  if (!v) return 0;
  trim_line(v);
  if (strlen(v) + 1 > cap || version_compare(v, TINYSHELL_VERSION) == VERSION_INVALID) {
    say("version.txt del release es inválido.\n");
    free(v);
    return 0;
  }
  memcpy(latest, v, strlen(v) + 1); /* length checked above */
  free(v);
  return 1;
}

static char *download(const char *suffix, size_t max, size_t *len) {
  Buf url = {0};
  char *body;
  buf_str(&url, UPDATE_BASE_URL);
  buf_str(&url, SYS_RELEASE_ASSET);
  buf_str(&url, suffix);
  body = http_get(url.p, NULL, max, len);
  free(url.p);
  return body;
}

/* Downloads, verifies and stages the new binary at p->fresh. 1 if ready to swap. */
static int stage(const Paths *p, const char *latest) {
  size_t exe_len = 0, sig_len = 0;
  char *exe = download("", MAX_UPDATE, &exe_len);
  char *sig = exe ? download(".sig", 2048, &sig_len) : NULL;
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
  if (!sys_write_file(p->fresh.p, exe, exe_len, 0)) {
    say("No se pudo escribir el binario nuevo (¿carpeta sin permiso de escritura?).\n");
    goto done;
  }
  if (!self_test(p->fresh.p, tested, sizeof tested) || version_compare(tested, latest) != 0 ||
      version_compare(tested, TINYSHELL_VERSION) <= 0) {
    say("El binario nuevo no pasó la prueba de arranque en este equipo. No se instaló nada.\n");
    sys_remove(p->fresh.p);
    goto done;
  }
  ok = 1;
done:
  free(exe);
  free(sig);
  return ok;
}

/* self -> old, fresh -> self; on failure the previous binary is put back. */
static int swap_in(const Paths *p) {
  if (!sys_rename(p->self.p, p->old.p)) {
    say("No se pudo apartar el binario actual (error %lu). No se instaló nada.\n", sys_error());
    sys_remove(p->fresh.p);
    return 0;
  }
  if (!sys_rename(p->fresh.p, p->self.p)) {
    say("No se pudo instalar el binario nuevo (error %lu); se restauró el anterior.\n", sys_error());
    sys_rename(p->old.p, p->self.p);
    sys_remove(p->fresh.p);
    return 0;
  }
  return 1;
}

static int run(Paths *p) {
  char latest[32], answer[64];
  if (!release_key_present()) {
    say("Este binario no trae la clave pública de releases: no puede verificar actualizaciones.\n");
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
  if (!stage(p, latest) || !swap_in(p)) return 1;
  say("Actualizado a %s. Cerrá y volvé a abrir TinyShell.\nSi algo falla: tinyshell --rollback\n", latest);
  return 0;
}

int update_run(void) {
  Paths p;
  int r;
  if (!paths_init(&p)) {
    say("No se pudo ubicar el ejecutable actual.\n");
    paths_free(&p);
    return 1;
  }
  r = run(&p);
  paths_free(&p);
  return r;
}

/* self -> fresh (temporary), old -> self, fresh -> old: a second rollback rolls forward. */
static int rollback(const Paths *p) {
  if (!sys_exists(p->old.p)) {
    say("No hay versión anterior guardada (%s).\n", p->old.p);
    return 1;
  }
  if (!sys_rename(p->self.p, p->fresh.p)) {
    say("No se pudo apartar el binario actual (error %lu).\n", sys_error());
    return 1;
  }
  if (!sys_rename(p->old.p, p->self.p)) {
    say("No se pudo restaurar la versión anterior (error %lu).\n", sys_error());
    sys_rename(p->fresh.p, p->self.p);
    return 1;
  }
  sys_rename(p->fresh.p, p->old.p);
  say("Versión anterior restaurada. Cerrá y volvé a abrir TinyShell.\n");
  return 0;
}

int update_rollback(void) {
  Paths p;
  int r;
  if (!paths_init(&p)) {
    say("No se pudo ubicar el ejecutable actual.\n");
    paths_free(&p);
    return 1;
  }
  r = rollback(&p);
  paths_free(&p);
  return r;
}
