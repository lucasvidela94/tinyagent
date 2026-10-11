/* Tests for src/platform/posix/credential_posix.c, run natively on Linux by
 * build/test-core.sh under a fresh temporary $XDG_CONFIG_HOME. */
#include "posix.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../check.h"
#include "credential.h"
#include "sys.h"

int main(void) {
  char root[] = "/tmp/tinyshell-cred-XXXXXX", key[64];
  Buf path = {0}, tmp = {0};
  struct stat st;
  int r;
  if (!mkdtemp(root)) return 1;
  setenv("XDG_CONFIG_HOME", root, 1);
  CHECK(!credential_load("opencode-go", key, sizeof key)); /* no file yet */
  CHECK(credential_delete("opencode-go"));                 /* deleting nothing is fine */
  CHECK(credential_save("opencode-go", "sk-uno"));
  CHECK(credential_save("openrouter", "sk-dos"));
  sys_data_path("credentials", &path);
  CHECK(stat(path.p, &st) == 0 && (st.st_mode & 0777) == 0600);
  buf_str(&tmp, path.p);
  buf_str(&tmp, ".tmp");
  CHECK(!sys_exists(tmp.p)); /* the temporary file was renamed over the real one */
  CHECK(credential_load("opencode-go", key, sizeof key) && !strcmp(key, "sk-uno"));
  CHECK(credential_delete("opencode-go"));
  CHECK(!credential_load("opencode-go", key, sizeof key));
  CHECK(credential_load("openrouter", key, sizeof key) && !strcmp(key, "sk-dos")); /* untouched */
  CHECK(!credential_save("x", "bad\nkey"));
  r = CHECK_DONE("test_credential_posix");
  unlink(path.p);
  path.p[strlen(path.p) - strlen("/credentials")] = 0;
  if (rmdir(path.p) || rmdir(root)) r = 1;
  free(path.p);
  free(tmp.p);
  return r;
}
