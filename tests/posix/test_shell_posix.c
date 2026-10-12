/* Tests for src/platform/posix/shell_posix.c and cancel_posix.c, run natively on Linux by
 * build/test-core.sh with a 2 s command timeout (CMD_TIMEOUT_MS=2000). */
#include "posix.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../check.h"
#include "cancel.h"
#include "config.h"
#include "shell.h"
#include "sys.h"

static int has(char *out, const char *part) {
  int ok = out && strstr(out, part) != NULL;
  free(out);
  return ok;
}

static void pause_ms(long ms) {
  struct timespec t;
  t.tv_sec = ms / 1000;
  t.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&t, NULL);
}

static int alive(pid_t pid) { /* gone within a second, or still alive */
  int i;
  for (i = 0; i < 20; i++) {
    if (kill(pid, 0) != 0 && errno == ESRCH) return 0;
    pause_ms(50);
  }
  return 1;
}

static void test_output_and_exit(void) {
  char *out = run_command("printf hola");
  CHECK(out && !strcmp(out, "hola\n[exit=0]"));
  free(out);
  CHECK(has(run_command("echo err >&2; exit 3"), "err\n\n[exit=3]")); /* stderr merged */
  out = run_command("cat"); /* stdin is empty: returns at once */
  CHECK(out && !strcmp(out, "(no output)\n[exit=0]"));
  free(out);
  CHECK(has(run_command("kill -9 $$"), "[exit=137]")); /* killed by a signal: 128 + 9 */
}

static void test_truncation(void) {
  char *out = run_command("head -c 100000 /dev/zero | tr '\\0' a");
  CHECK(out && strstr(out, "[output truncated: ") && strspn(out, "a") == MAX_OUTPUT);
  free(out);
}

static void test_timeout_kills_the_tree(void) {
  unsigned long t0 = sys_ms();
  char *out = run_command("sleep 30 & echo $!; wait");
  pid_t grandchild = out ? (pid_t)atol(out) : 0;
  CHECK(out && strstr(out, "[timed out"));
  CHECK(sys_ms() - t0 < 4000);
  CHECK(grandchild > 0 && !alive(grandchild));
  free(out);
}

static void test_background_child_does_not_block(void) {
  unsigned long t0 = sys_ms();
  char *out = run_command("sleep 30 & echo $!; echo listo");
  pid_t bg = out ? (pid_t)atol(out) : 0;
  CHECK(out && strstr(out, "listo") && strstr(out, "[exit=0]"));
  CHECK(sys_ms() - t0 < 1500); /* the 500 ms grace, not the 2 s timeout */
  if (bg > 0) kill(bg, SIGKILL);
  free(out);
}

static void test_ctrl_c(void) {
  pid_t helper = fork();
  unsigned long t0;
  char *out;
  if (helper == 0) { /* sends Ctrl+C while the command runs */
    pause_ms(300);
    kill(getppid(), SIGINT);
    _exit(0);
  }
  t0 = sys_ms();
  out = run_command("sleep 30");
  CHECK(out && strstr(out, "[cancelled by the user with Ctrl+C]"));
  CHECK(sys_ms() - t0 < 1500);
  CHECK(cancel_requested());
  CHECK(!cancel_take_input_interrupt()); /* it stopped the command, not a prompt */
  free(out);
  waitpid(helper, NULL, 0);
  cancel_reset();
  CHECK(!cancel_requested());
  raise(SIGINT); /* nothing running: it ends the pending prompt instead */
  CHECK(cancel_requested() && cancel_take_input_interrupt() && !cancel_take_input_interrupt());
  cancel_reset();
}

static void test_latin1_output(void) {
  unsetenv("LC_ALL");
  unsetenv("LC_CTYPE");
  setenv("LANG", "es_ES.ISO-8859-1", 1);
  CHECK(has(run_command("printf 'a\\361o'"), "a\xc3\xb1o")); /* Latin-1 "ñ" becomes UTF-8 */
  setenv("LANG", "es_AR.UTF-8", 1);
  CHECK(has(run_command("printf 'a\\303\\261o'"), "a\xc3\xb1o")); /* UTF-8 passes through */
}

int main(void) {
  cancel_init();
  test_output_and_exit();
  test_truncation();
  test_timeout_kills_the_tree();
  test_background_child_does_not_block();
  test_ctrl_c();
  test_latin1_output();
  return CHECK_DONE("test_shell_posix");
}
