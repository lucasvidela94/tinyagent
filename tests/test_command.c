/* Regression tests for src/core/command.c: the command result text the model reads.
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "command.h"

static int result_is(char *got, const char *want) {
  int ok = got && !strcmp(got, want);
  free(got);
  return ok;
}

int main(void) {
  CHECK(result_is(command_result("hola\n", 0, 0, 0, 0), "hola\n\n[exit=0]"));
  CHECK(result_is(command_result("", 0, 0, 0, 2), "(sin salida)\n[exit=2]"));
  CHECK(result_is(command_result("x", 10, 0, 0, 0), "x\n[salida truncada: 10 bytes omitidos]\n[exit=0]"));
  CHECK(result_is(command_result("x", 0, 1, 0, 1), "x\n[tiempo agotado: terminado tras 180 s]"));
  /* Ctrl+C wins over a timeout: the user stopped it, whatever else happened. */
  CHECK(result_is(command_result("x", 0, 1, 1, 1), "x\n[cancelado por el usuario con Ctrl+C]"));
  return CHECK_DONE("test_command");
}
