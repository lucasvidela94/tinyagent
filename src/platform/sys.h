/* Operating system primitives: the platform seam for everything that is not console, HTTP,
 * shell, credentials, settings or Ctrl+C. One adapter per platform (sys_win32.c). Every
 * path is UTF-8; the adapter converts to the OS encoding. */
#ifndef TINYSHELL_SYS_H
#define TINYSHELL_SYS_H

#include <stddef.h>

#include "util.h"

extern const char SYS_EOL[]; /* line ending for text files the user may open ("\r\n" on Windows) */
extern const char SYS_RELEASE_ASSET[]; /* this platform's binary in a release ("tinyshell.exe") */

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
 * Files that report no size (Linux /proc) are read to the end, keeping the first `max`
 * bytes. NULL if it cannot be read. Caller frees. */
char *sys_read_file(const char *path, size_t max, size_t *len);
/* Creates or replaces the file (SYS_APPEND: adds to the end). New files are private to the
 * user where the OS has permissions; SYS_EXECUTABLE also makes them runnable. 1 on success. */
enum { SYS_APPEND = 1, SYS_EXECUTABLE = 2 };
int sys_write_file(const char *path, const char *data, size_t n, int flags);

/* For the updater. Each returns 1 on success; on failure sys_error() has the OS code. */
int sys_self_path(Buf *out);                   /* appends the running executable's path */
int sys_rename(const char *from, const char *to); /* replaces `to`; works on a running binary */
int sys_remove(const char *path);
int sys_exists(const char *path);
unsigned long sys_error(void); /* OS error code of the last failed call, for messages */

#endif
