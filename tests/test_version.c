/* Regression tests for src/core/version.c: release version comparison and parsing of the
 * self-test output used by the updater. Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "version.h"
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

static int sibling_is(const char *self, const char *tag, const char *want) {
  Buf b = {0};
  int ok;
  version_sibling(self, tag, &b);
  ok = b.p && !strcmp(b.p, want);
  free(b.p);
  return ok;
}

static void test_sibling(void) {
  CHECK(sibling_is("C:\\x\\tinyshell.exe", "new", "C:\\x\\tinyshell.new.exe"));
  CHECK(sibling_is("C:\\X\\TINYSHELL.EXE", "old", "C:\\X\\TINYSHELL.old.EXE")); /* still ends in .EXE */
  CHECK(sibling_is("/home/u/.local/bin/tinyshell", "new", "/home/u/.local/bin/tinyshell.new"));
  CHECK(sibling_is("/opt/v1.exe.d/tinyshell", "old", "/opt/v1.exe.d/tinyshell.old")); /* .exe only at the end */
  CHECK(sibling_is(".exe", "new", ".exe.new")); /* no name before the extension */
}

int main(void) {
  test_compare();
  test_from_output();
  test_sibling();
  return CHECK_DONE("test_version");
}
