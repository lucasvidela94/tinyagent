/* Regression tests for src/core/instructions.c with both dialects: each platform's prompt
 * names its own shell and tools and none of the other's. Run: ./build/test-core.sh */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "config.h"
#include "instructions.h"

static int has(const char *s, const char *part) { return strstr(s, part) != NULL; }

int main(void) {
  char *win = instructions(&DIALECT_CMD), *lin = instructions(&DIALECT_SH), limits[64];
  snprintf(limits, sizeof limits, "stopped after %d seconds", CMD_TIMEOUT_MS / 1000);
  CHECK(has(win, "Windows PC") && has(win, "one cmd.exe command") && has(win, "batch scripts"));
  CHECK(has(win, "reg query") && has(win, "registry"));
  CHECK(!has(win, "/bin/sh") && !has(win, "Linux") && !has(win, "sudo"));
  CHECK(has(lin, "Linux PC") && has(lin, "one /bin/sh command") && has(lin, "shell scripts"));
  CHECK(has(lin, "busybox") && has(lin, "packages") && has(lin, "sudo"));
  CHECK(!has(lin, "cmd.exe") && !has(lin, "Windows") && !has(lin, "wmic") && !has(lin, "registry"));
  CHECK(has(win, limits) && has(lin, limits)); /* limits come from config.h */
  CHECK(has(win, "approves every command") && has(lin, "approves every command"));
  free(win);
  free(lin);
  return CHECK_DONE("test_instructions");
}
