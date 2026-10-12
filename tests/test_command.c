/* Regression tests for src/core/command.c: the command result text the model reads.
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "command.h"
#include "i18n.h"

static int result_is(char *got, const char *want) {
  int ok = got && !strcmp(got, want);
  free(got);
  return ok;
}

int main(void) {
  CHECK(result_is(command_result("hola\n", 0, 0, 0, 0), "hola\n\n[exit=0]"));
  CHECK(result_is(command_result("", 0, 0, 0, 2), "(no output)\n[exit=2]"));
  CHECK(result_is(command_result("x", 10, 0, 0, 0), "x\n[output truncated: 10 bytes left out]\n[exit=0]"));
  CHECK(result_is(command_result("x", 0, 1, 0, 1), "x\n[timed out: stopped after 180 s]"));
  /* Ctrl+C wins over a timeout: the user stopped it, whatever else happened. */
  CHECK(result_is(command_result("x", 0, 1, 1, 1), "x\n[cancelled by the user with Ctrl+C]"));
  i18n_set(LANG_ES); /* the notes follow the interface language */
  CHECK(result_is(command_result("", 10, 0, 0, 3), "(sin salida)\n[salida truncada: 10 bytes omitidos]\n[c\xc3\xb3""digo de salida 3]"));
  CHECK(result_is(command_result("x", 0, 1, 0, 1), "x\n[tiempo agotado: terminado tras 180 s]"));
  i18n_set(LANG_EN);
  return CHECK_DONE("test_command");
}
