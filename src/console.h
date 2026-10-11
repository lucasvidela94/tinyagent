/* Console text I/O in UTF-8 on top of the wide console API (Win32 adapter).
 * Code page 65001 is unreliable on the Windows 7 console: non-ASCII input reads as
 * EOF and output can repeat, so the program never switches code pages. */
#ifndef TINYSHELL_CONSOLE_H
#define TINYSHELL_CONSOLE_H

#include <stddef.h>

enum { LINE_EOF = 0, LINE_OK = 1, LINE_TOO_LONG = 2 };
enum { COLOR_NORMAL, COLOR_DIM, COLOR_ACCENT, COLOR_DANGER, COLOR_USER };

void console_init(void);
void say(const char *fmt, ...);               /* printf-style, UTF-8, default color */
void console_write(const char *s, int color); /* color is ignored when not a console */

/* Screen primitives for the TUI. Only meaningful when console_is_tty(). Rows and columns
 * are screen-buffer coordinates; lines wrap at console_width(). */
int console_is_tty(void); /* both stdin and stdout are a console */
int console_width(void);
int console_row(void);
void console_goto(int row, int col);
void console_clear(int from_row, int to_row); /* blank rows in the default color */
void console_flush_input(void);               /* drop pending keystrokes (e.g. a multi-line paste) */

/* Reads one line as UTF-8 into out (trailing blanks trimmed). On LINE_TOO_LONG the
 * excess has been consumed, so it never leaks into the next prompt (e.g. the approval).
 * echo = 0 hides typing (API key). */
int read_line(char *out, size_t cap, int echo);

#endif
