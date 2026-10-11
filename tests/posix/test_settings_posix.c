/* Tests for src/platform/posix/settings_posix.c, run natively on Linux by
 * build/test-core.sh under a fresh temporary $XDG_CONFIG_HOME. */
#include "posix.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../check.h"
#include "console.h"
#include "settings.h"
#include "sys.h"

int main(void) {
  char root[] = "/tmp/tinyshell-set-XXXXXX", v[64];
  Provider p;
  Buf cfg = {0};
  char *text;
  int r;
  if (!mkdtemp(root)) return 1;
  setenv("XDG_CONFIG_HOME", root, 1);
  console_init();
  settings_load(&p); /* first run: writes the commented template */
  CHECK(!strcmp(p.name, "opencode-go") && !strcmp(p.model, "deepseek-v4.1-flash"));
  CHECK(strstr(settings_path(), "/tinyshell/config.ini") != NULL);
  CHECK(settings_save_model("kimi-k2.7-code"));
  CHECK(settings_set("telemetry", "enabled", "0"));
  settings_load(&p);
  CHECK(!strcmp(p.model, "kimi-k2.7-code"));
  CHECK(settings_get("telemetry", "enabled", v, sizeof v) && !strcmp(v, "0"));
  CHECK(!settings_get("telemetry", "noticed", v, sizeof v) && v[0] == 0);
  sys_data_path("config.ini", &cfg);
  text = sys_read_file(cfg.p, 65536, NULL);
  CHECK(text && strstr(text, "; Ejemplos:") != NULL); /* the user's comments survive edits */
  free(text);
  sys_write_file(cfg.p, "[provider]\nmodel = con espacio\n", 31, 0); /* invalid value: default kept */
  settings_load(&p);
  CHECK(!strcmp(p.model, "deepseek-v4.1-flash"));
  r = CHECK_DONE("test_settings_posix");
  unlink(cfg.p);
  cfg.p[strlen(cfg.p) - strlen("/config.ini")] = 0;
  if (rmdir(cfg.p) || rmdir(root)) r = 1;
  free(cfg.p);
  return r;
}
