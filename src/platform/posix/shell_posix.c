#include "posix.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cancel.h"
#include "cancel_posix.h"
#include "charset.h"
#include "command.h"
#include "config.h"
#include "i18n.h"
#include "shell.h"
#include "sys.h"
#include "util.h"

/* Child: own process group (so the terminal's Ctrl+C reaches TinyShell, which then kills
 * the whole group), stdin from /dev/null, stdout and stderr into the pipe. */
static void child(const char *cmd, int out) {
  int in = open("/dev/null", O_RDONLY);
  setpgid(0, 0);
  signal(SIGINT, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);
  if (in >= 0) dup2(in, 0);
  dup2(out, 1);
  dup2(out, 2);
  execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
  _exit(127);
}

/* Drains the pipe until every writer is gone, the shell has exited (plus a grace period for
 * background children holding the pipe), or the timeout hits. Returns 1 on timeout. */
static int collect(int rd, pid_t pid, Buf *out, size_t *dropped, int *status, int *exited) {
  unsigned long start = sys_ms(), exited_at = 0;
  for (;;) {
    struct pollfd p;
    char buf[4096];
    int ready;
    p.fd = rd;
    p.events = POLLIN;
    ready = poll(&p, 1, 50);
    if (ready > 0) {
      ssize_t got = read(rd, buf, sizeof buf);
      if (got > 0) {
        buf_add_capped(out, buf, (size_t)got, MAX_OUTPUT, dropped); /* keep reading: the child never blocks */
        continue;
      }
      if (got == 0 || errno != EINTR) return 0; /* EOF: every writer closed the pipe */
    }
    if (cancel_requested()) return 0;
    if (sys_ms() - start > CMD_TIMEOUT_MS) return 1;
    if (!*exited && waitpid(pid, status, WNOHANG) == pid) *exited = 1;
    if (*exited) {
      if (!exited_at) exited_at = sys_ms();
      else if (sys_ms() - exited_at > 500) return 0;
    }
  }
}

/* Waits up to 5 s for a shell that closed its output but kept running, then kills its group. */
static void reap(pid_t pid, int *status) {
  unsigned long start = sys_ms();
  struct timespec tick = {0, 50000000};
  while (waitpid(pid, status, WNOHANG) == 0) {
    if (sys_ms() - start > 5000) {
      kill(-pid, SIGKILL);
      waitpid(pid, status, 0);
      return;
    }
    nanosleep(&tick, NULL);
  }
}

/* Output in a Latin-1 locale is Latin-1 bytes; UTF-8 is passed on as it is. */
static char *to_utf8(Buf *out) {
  Buf u = {0};
  size_t k;
  const char *loc = getenv("LC_ALL");
  for (k = 0; k < out->len; k++)
    if (!out->p[k]) out->p[k] = ' '; /* NUL bytes would cut the text short */
  if (!loc || !*loc) loc = getenv("LC_CTYPE");
  if (!loc || !*loc) loc = getenv("LANG");
  if (out->len && !utf8_valid(out->p) && charset_from_locale(loc) == CHARSET_LATIN1) {
    charset_decode(out->p, out->len, CHARSET_LATIN1, &u);
    free(out->p);
    return buf_take(&u);
  }
  return buf_take(out);
}

char *run_command(const char *cmd) {
  int fds[2], status = 0, exited = 0, timed_out;
  size_t dropped = 0;
  Buf out = {0};
  char *text, *res;
  pid_t pid;
  if (pipe(fds)) return xstrdup(tr("Could not create the output pipe. Nothing was run."));
  pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return xstrdup(tr("Could not start /bin/sh. Nothing was run."));
  }
  if (pid == 0) {
    close(fds[0]);
    child(cmd, fds[1]);
  }
  close(fds[1]); /* keep only the child's copy so the read side sees EOF */
  setpgid(pid, pid); /* also here: no race with Ctrl+C before the child runs setpgid */
  cancel_watch_process(pid);
  timed_out = collect(fds[0], pid, &out, &dropped, &status, &exited);
  cancel_release_process();
  if (timed_out) kill(-pid, SIGKILL);
  close(fds[0]);
  if (!exited) reap(pid, &status);
  text = to_utf8(&out);
  res = command_result(text, dropped, timed_out, cancel_requested(),
                       WIFEXITED(status) ? (unsigned long)WEXITSTATUS(status) : 128ul + (unsigned long)WTERMSIG(status));
  free(text);
  return res;
}
