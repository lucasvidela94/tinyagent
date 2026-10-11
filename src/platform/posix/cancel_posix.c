#include "posix.h"

#include <signal.h>
#include <string.h>
#include <sys/socket.h>

#include "cancel.h"
#include "cancel_posix.h"

static volatile sig_atomic_t requested, input_interrupted, group, sock = -1;

/* Installed without SA_RESTART: a read() blocked on the terminal returns EINTR, so a
 * pending prompt ends without injecting keys (the Windows adapter has to). */
static void on_sigint(int sig) {
  (void)sig;
  requested = 1;
  if (group > 0) kill(-(pid_t)group, SIGKILL);
  else if (sock >= 0) shutdown((int)sock, SHUT_RDWR);
  else input_interrupted = 1;
}

void cancel_init(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  signal(SIGPIPE, SIG_IGN); /* a peer closing a socket or pipe is an error to handle, not a crash */
}

int cancel_requested(void) { return requested != 0; }
void cancel_reset(void) { requested = 0; }

int cancel_take_input_interrupt(void) {
  int was = input_interrupted != 0;
  input_interrupted = 0;
  return was;
}

/* Ctrl+C may arrive just before registering: then act on it at once. */
void cancel_watch_process(pid_t g) {
  group = (sig_atomic_t)g;
  if (requested) kill(-g, SIGKILL);
}

void cancel_release_process(void) { group = 0; }

void cancel_watch_socket(int fd) {
  sock = fd;
  if (requested) shutdown(fd, SHUT_RDWR);
}

void cancel_release_socket(void) { sock = -1; }
