/* Regression tests for command_prefix() and the null-device redirections in risk_reason()
 * (src/core/policy.c): which commands the user may approve "always", under what prefix.
 * Run: ./build/test-core.sh */
#include <string.h>

#include "dialect.h"
#include "policy.h"
#include "check.h"

static int prefix_is(const Dialect *d, const char *cmd, const char *want) {
  char p[64];
  return command_prefix(d, cmd, p, sizeof p) && !strcmp(p, want);
}

static int no_prefix(const Dialect *d, const char *cmd) {
  char p[64];
  return !command_prefix(d, cmd, p, sizeof p);
}

static void test_cmd(void) {
  const Dialect *d = &DIALECT_CMD;
  CHECK(prefix_is(d, "reg query \"HKLM\\SOFTWARE\\x\" /s /v DisplayName", "reg query"));
  CHECK(prefix_is(d, "REG.exe QUERY HKCU\\x", "reg query")); /* cmd ignores case and .exe */
  CHECK(prefix_is(d, "dir /a C:\\", "dir"));
  CHECK(prefix_is(d, "dir \"C:\\Program Files\" 2>nul", "dir")); /* the null device is harmless */
  CHECK(prefix_is(d, "whoami /groups", "whoami"));
  CHECK(prefix_is(d, "  systeminfo", "systeminfo"));
  CHECK(no_prefix(d, "reg delete HKCU\\x /f"));          /* risky: once only */
  CHECK(no_prefix(d, "reg /?"));                         /* a subcommand tool needs its subcommand */
  CHECK(no_prefix(d, "dir & del x"));                    /* chained */
  CHECK(no_prefix(d, "dir > lista.txt"));                /* writes a file */
  CHECK(no_prefix(d, "type x | findstr y"));             /* piped */
  CHECK(no_prefix(d, "cmd /c dir"));                     /* runs other commands */
  CHECK(no_prefix(d, "start notepad"));
  CHECK(no_prefix(d, "C:\\tools\\dir.exe"));             /* a path could name any program */
  CHECK(no_prefix(d, "del x"));
  CHECK(no_prefix(d, ""));
  CHECK(risk_reason(d, "dir x 2>nul") == NULL && risk_reason(d, "dir x >nul 2>&1") == NULL);
  CHECK(risk_reason(d, "dir x > nul.txt") != NULL); /* a real file */
  CHECK(risk_reason(d, "dir x > out.txt") != NULL);
}

static void test_sh(void) {
  const Dialect *d = &DIALECT_SH;
  char p[8];
  CHECK(prefix_is(d, "git status --porcelain", "git status"));
  CHECK(prefix_is(d, "ls -la /etc", "ls"));
  CHECK(prefix_is(d, "df -h 2>/dev/null", "df"));
  CHECK(prefix_is(d, "LS -la", "LS")); /* sh is case-sensitive: LS keeps its case, not ls */
  CHECK(no_prefix(d, "git -C x push"));
  CHECK(no_prefix(d, "ls; rm x"));
  CHECK(no_prefix(d, "echo $(id)"));
  CHECK(no_prefix(d, "sudo apt-get update"));
  CHECK(no_prefix(d, "find / -name x -delete"));
  CHECK(no_prefix(d, "./script.sh"));
  CHECK(no_prefix(d, "rm -rf x"));
  CHECK(!command_prefix(d, "systemctl status sshd", p, sizeof p)); /* does not fit: refused, not cut */
  CHECK(risk_reason(d, "ls x 2>/dev/null") == NULL && risk_reason(d, "ls x > /dev/null 2>&1") == NULL);
}

int main(void) {
  test_cmd();
  test_sh();
  return CHECK_DONE("test_prefix");
}
