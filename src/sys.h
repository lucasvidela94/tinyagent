/* Operating system primitives: the platform seam for everything that is not console, HTTP,
 * shell, credentials, settings or Ctrl+C. One adapter per platform (sys_win32.c). Every
 * path is UTF-8; the adapter converts to the OS encoding. */
#ifndef TINYSHELL_SYS_H
#define TINYSHELL_SYS_H

#include <stddef.h>

#include "util.h"

extern const char SYS_EOL[]; /* line ending for text files the user may open ("\r\n" on Windows) */

/* n bytes from the OS random generator; a weak time-based fill only if it is unavailable.
 * Good for IDs, not for keys. */
void sys_random(unsigned char *out, size_t n);
unsigned long sys_ms(void); /* monotonic milliseconds; wraps, so only differences matter */

void sys_cwd(Buf *out); /* appends the current directory, nothing if unknown */

/* Hostname, user name and home folder, appended (empty if unknown). Used to redact them. */
void sys_identity(Buf *host, Buf *user, Buf *home);

/* Appends the path of <data dir>/<name> (Windows: %LOCALAPPDATA%\TinyShell). `name` may
 * contain '/'; the data dir and any folders in `name` are created. 0 on failure. */
int sys_data_path(const char *name, Buf *out);

/* The last `max` bytes of a file (all of it if smaller), NUL-terminated, length in *len.
 * NULL if it cannot be read. Caller frees. */
char *sys_read_file(const char *path, size_t max, size_t *len);
int sys_write_file(const char *path, const char *data, size_t n, int append); /* 1 on success */

#endif
