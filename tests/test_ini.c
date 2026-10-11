/* Regression tests for src/core/ini.c: config.ini reading and editing on Linux.
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "ini.h"

static const char *CONFIG = "; comentario\r\n"
                            "[provider]\r\n"
                            "name = opencode-go\r\n"
                            "  Model=deepseek  \r\n"
                            "session_header =\r\n"
                            "[telemetry]\r\n"
                            "enabled = 1\r\n";

static char *set(const char *text, const char *s, const char *k, const char *v) {
  Buf b = {0};
  if (!ini_set(text, s, k, v, &b)) {
    free(b.p);
    return NULL;
  }
  return buf_take(&b);
}

static void test_get(void) {
  char v[64];
  CHECK(ini_get(CONFIG, "provider", "name", v, sizeof v) && !strcmp(v, "opencode-go"));
  CHECK(ini_get(CONFIG, "PROVIDER", "model", v, sizeof v) && !strcmp(v, "deepseek")); /* case, spaces */
  CHECK(ini_get(CONFIG, "provider", "session_header", v, sizeof v) && !strcmp(v, "")); /* empty exists */
  CHECK(!ini_get(CONFIG, "provider", "enabled", v, sizeof v)); /* that key is in another section */
  CHECK(ini_get(CONFIG, "telemetry", "enabled", v, sizeof v) && !strcmp(v, "1"));
  CHECK(!ini_get(CONFIG, "provider", "url", v, sizeof v));
  CHECK(!ini_get("; name = x\n", "", "name", v, sizeof v)); /* comments are not keys */
  CHECK(!ini_get(CONFIG, "provider", "name", v, 4));         /* does not fit */
}

static void test_set(void) {
  char v[64], *t = set(CONFIG, "provider", "model", "kimi"), *u;
  CHECK(t && ini_get(t, "provider", "model", v, sizeof v) && !strcmp(v, "kimi"));
  CHECK(t && strstr(t, "; comentario\n[provider]\nname = opencode-go\nmodel = kimi\n")); /* kept, in place */
  u = set(t, "provider", "url", "https://x"); /* new key at the end of its section */
  CHECK(u && strstr(u, "session_header =\nurl = https://x\n[telemetry]"));
  free(t);
  t = set(u, "telemetry", "install_id", "ab12"); /* last section: appended */
  CHECK(t && !strcmp(t + strlen(t) - strlen("enabled = 1\ninstall_id = ab12\n"), "enabled = 1\ninstall_id = ab12\n"));
  free(u);
  u = set(t, "nueva", "k", "v"); /* missing section is created */
  CHECK(u && ini_get(u, "nueva", "k", v, sizeof v) && !strcmp(v, "v"));
  free(t);
  free(u);
  t = set("", "provider", "name", "x");
  CHECK(t && !strcmp(t, "[provider]\nname = x\n"));
  free(t);
  CHECK(set(CONFIG, "provider", "model", "a\nurl = http://evil") == NULL); /* no line injection */
  CHECK(set(CONFIG, "p]", "k", "v") == NULL);
  CHECK(set(CONFIG, "provider", "k=1", "v") == NULL);
}

int main(void) {
  test_get();
  test_set();
  return CHECK_DONE("test_ini");
}
