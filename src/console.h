/* Console text I/O in UTF-8 on top of the wide console API (Win32 adapter).
 * Code page 65001 is unreliable on the Windows 7 console: non-ASCII input reads as
 * EOF and output can repeat, so the program never switches code pages. */
#ifndef TINYSHELL_CONSOLE_H
#define TINYSHELL_CONSOLE_H

#include <stddef.h>

enum { LINE_EOF = 0, LINE_OK = 1, LINE_TOO_LONG = 2 };

void console_init(void);
void say(const char *fmt, ...); /* printf-style, UTF-8 */

/* Reads one line as UTF-8 into out (trailing blanks trimmed). On LINE_TOO_LONG the
 * excess has been consumed, so it never leaks into the next prompt (e.g. the approval).
 * echo = 0 hides typing (API key). */
int read_line(char *out, size_t cap, int echo);

#endif
