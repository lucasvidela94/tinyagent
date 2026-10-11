/* Regression tests for src/version.c: release version comparison and parsing of the
 * self-test output used by the updater. Run: ./build/test-core.sh */
#include <string.h>

#include "../src/version.h"
#include "check.h"

static void test_compare(void) {
  CHECK(version_compare("0.4.0", "0.4.0") == 0);
  CHECK(version_compare("v0.4.0", "0.4.0") == 0);
  CHECK(version_compare("0.4.1", "0.4.0") == 1);
  CHECK(version_compare("0.4.0", "0.10.0") == -1); /* numeric, not lexical */
  CHECK(version_compare("1.0.0", "0.99.99") == 1);
  CHECK(version_compare("0.4.0\r\n", "0.4.0") == 0); /* version.txt may end in CRLF */
  CHECK(version_compare("", "0.4.0") == VERSION_INVALID);
  CHECK(version_compare("0.4", "0.4.0") == VERSION_INVALID);
  CHECK(version_compare("0.4.0.1", "0.4.0") == VERSION_INVALID);
  CHECK(version_compare("0.4.x", "0.4.0") == VERSION_INVALID);
  CHECK(version_compare("<html>", "0.4.0") == VERSION_INVALID);
}

static void test_from_output(void) {
  char v[32];
  CHECK(version_from_output("tinyshell 0.4.1\n[exit=0]", v, sizeof v) && !strcmp(v, "0.4.1"));
  CHECK(!version_from_output("TinyShell 0.4.1\n", v, sizeof v));
  CHECK(!version_from_output("tinyshell abc\n", v, sizeof v));
  CHECK(!version_from_output("No se pudo iniciar cmd.exe", v, sizeof v));
  CHECK(!version_from_output("tinyshell 0.4.1", v, 3)); /* does not fit */
}

int main(void) {
  test_compare();
  test_from_output();
  return CHECK_DONE("test_version");
}
