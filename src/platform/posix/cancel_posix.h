/* What Ctrl+C must stop, registered by the other POSIX adapters (private to posix/).
 * The SIGINT handler only does async-signal-safe things: set flags, kill(), shutdown(). */
#ifndef TINYSHELL_CANCEL_POSIX_H
#define TINYSHELL_CANCEL_POSIX_H

#include "posix.h"

#include <sys/types.h>

/* A running command's process group (its leader's pid); Ctrl+C kills the whole group. */
void cancel_watch_process(pid_t group);
void cancel_release_process(void);

/* A connected socket; Ctrl+C shuts it down so a blocked read or write fails at once. */
void cancel_watch_socket(int fd);
void cancel_release_socket(void);

#endif
