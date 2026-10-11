/* Regression tests for src/core/credfile.c: the Linux API key file format.
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "credfile.h"

static char *set(const char *text, const char *provider, const char *key) {
  Buf b = {0};
  if (!credfile_set(text, provider, key, &b)) {
    free(b.p);
    return NULL;
  }
  return buf_take(&b);
}

int main(void) {
  char key[32], *t, *u;
  t = set("", "opencode-go", "sk-uno");
  CHECK(t && !strcmp(t, "opencode-go\tsk-uno\n"));
  u = set(t, "openrouter", "sk-dos"); /* another provider is added, the first kept */
  CHECK(u && credfile_get(u, "opencode-go", key, sizeof key) && !strcmp(key, "sk-uno"));
  CHECK(credfile_get(u, "openrouter", key, sizeof key) && !strcmp(key, "sk-dos"));
  free(t);
  t = set(u, "opencode-go", "sk-nueva"); /* replaced, not duplicated */
  CHECK(t && !strcmp(t, "openrouter\tsk-dos\nopencode-go\tsk-nueva\n"));
  free(u);
  u = set(t, "openrouter", NULL); /* logout removes only that line */
  CHECK(u && !strcmp(u, "opencode-go\tsk-nueva\n"));
  CHECK(!credfile_get(u, "openrouter", key, sizeof key) && key[0] == 0);
  free(t);
  free(u);
  /* "open" must not match the line of "opencode-go" */
  CHECK(!credfile_get("opencode-go\tsk-x\n", "open", key, sizeof key));
  CHECK(credfile_get("a\tk1\r\nb\tk2", "b", key, sizeof key) && !strcmp(key, "k2")); /* CRLF, no final break */
  CHECK(!credfile_get("a\t0123456789\n", "a", key, 5)); /* does not fit */
  CHECK(set("", "a", "k\nb\tevil") == NULL);             /* a key cannot inject a line */
  CHECK(set("", "a\tb", "k") == NULL);
  CHECK(set("", "a", "") == NULL);
  return CHECK_DONE("test_credfile");
}
