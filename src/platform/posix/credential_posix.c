#include "posix.h"

#include <stdlib.h>
#include <string.h>

#include "credential.h"
#include "credfile.h"
#include "sys.h"

#define MAX_CREDFILE (64 * 1024)

const char CREDENTIAL_WHERE[] = "~/.config/tinyshell/credentials (solo tu usuario puede leerlo)";

static char *read_all(Buf *path) {
  return sys_data_path("credentials", path) ? sys_read_file(path->p, MAX_CREDFILE, NULL) : NULL;
}

static void forget(char *s) { /* the file's text holds keys */
  if (s) wipe(s, strlen(s));
  free(s);
}

int credential_load(const char *provider, char *out, size_t cap) {
  Buf path = {0};
  char *text = read_all(&path);
  int ok = text && credfile_get(text, provider, out, cap);
  forget(text);
  free(path.p);
  return ok;
}

/* Written to a temporary file (0600) and renamed over the old one, so a crash or power
 * cut never leaves half a file. */
static int store(const char *provider, const char *key) {
  Buf path = {0}, tmp = {0}, next = {0};
  char *text = read_all(&path);
  int ok = path.p && credfile_set(text, provider, key, &next);
  if (ok) {
    buf_str(&tmp, path.p);
    buf_str(&tmp, ".tmp");
    ok = sys_write_file(tmp.p, next.p ? next.p : "", next.len, 0) && sys_rename(tmp.p, path.p);
    if (!ok) sys_remove(tmp.p);
  }
  forget(text);
  forget(next.p);
  free(path.p);
  free(tmp.p);
  return ok;
}

int credential_save(const char *provider, const char *key) { return store(provider, key); }
int credential_delete(const char *provider) { return store(provider, NULL); }
