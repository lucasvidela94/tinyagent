/* Console output on a VT100-style terminal (Linux console, xterm and the like): ANSI
 * escapes for color and cursor, termios for the cursor-position query. Rows are screen
 * rows; the TUI reserves its rows before drawing, so scrolling never moves them. */
#include "posix.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "charset.h"
#include "console.h"
#include "console_posix.h"
#include "util.h"

static int in_tty, out_tty, vt, charset = CHARSET_UTF8;

int console_charset(void) { return charset; }

void console_init(void) {
  const char *term = getenv("TERM"), *loc = getenv("LC_ALL");
  in_tty = isatty(0);
  out_tty = isatty(1);
  vt = out_tty && term && *term && strcmp(term, "dumb") != 0; /* escapes are understood */
  if (!loc || !*loc) loc = getenv("LC_CTYPE");
  if (!loc || !*loc) loc = getenv("LANG");
  charset = charset_from_locale(loc);
}

int console_is_tty(void) { return in_tty && vt; }

static void out_raw(const char *s, size_t n) {
  while (n) {
    ssize_t w = write(1, s, n);
    if (w <= 0) return;
    s += w;
    n -= (size_t)w;
  }
}

static void out_str(const char *s) { out_raw(s, strlen(s)); }

/* What the terminal's charset cannot show is replaced (see charset.h); redirected output
 * stays UTF-8. */
static void write_text(const char *s) {
  Buf b = {0};
  if (!out_tty || charset == CHARSET_UTF8) {
    out_str(s);
    return;
  }
  charset_encode(s, charset, &b);
  if (b.p) out_raw(b.p, b.len);
  free(b.p);
}

/* Bold yellow/red and reverse video: the Linux text console has them all. */
static const char *sgr_for(int color) {
  switch (color) {
  case COLOR_DIM: return "\033[2m";
  case COLOR_ACCENT: return "\033[1;33m";
  case COLOR_DANGER: return "\033[1;31m";
  case COLOR_USER: return "\033[7m";
  default: return "";
  }
}

void console_write(const char *s, int color) {
  int colored = vt && color != COLOR_NORMAL;
  if (colored) out_str(sgr_for(color));
  write_text(s);
  if (colored) out_str("\033[0m");
}

void say(const char *fmt, ...) {
  va_list ap, ap2;
  int n;
  char *s;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n >= 0) {
    s = (char *)xrealloc(NULL, (size_t)n + 1);
    vsnprintf(s, (size_t)n + 1, fmt, ap2);
    write_text(s);
    free(s);
  }
  va_end(ap2);
}

int console_width(void) {
  struct winsize ws;
  const char *cols = getenv("COLUMNS");
  if (out_tty && ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
  return cols && atoi(cols) > 0 ? atoi(cols) : 80;
}

/* Asks the terminal where the cursor is (ESC[6n -> ESC[row;colR). Keystrokes typed ahead
 * are dropped while waiting; 0 if the terminal does not answer within 200 ms. */
int console_row(void) {
  struct termios saved, raw;
  char reply[32];
  size_t n = 0;
  int row = 0, col = 0;
  if (!console_is_tty() || tcgetattr(0, &saved)) return 0;
  raw = saved;
  raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 2;
  tcsetattr(0, TCSANOW, &raw);
  out_str("\033[6n");
  while (n < sizeof reply - 1) {
    ssize_t r = read(0, reply + n, 1);
    if (r <= 0) break;
    if (reply[n] == '\033') { /* start of the answer: anything before it was typed ahead */
      reply[0] = '\033';
      n = 0;
    }
    if (reply[n++] == 'R') break;
  }
  reply[n] = 0;
  tcsetattr(0, TCSANOW, &saved);
  if (sscanf(reply, "\033[%d;%dR", &row, &col) != 2) return 0;
  return row - 1;
}

void console_goto(int row, int col) {
  char seq[32];
  if (!vt) return;
  snprintf(seq, sizeof seq, "\033[%d;%dH", (row < 0 ? 0 : row) + 1, col + 1);
  out_str(seq);
}

void console_clear(int from_row, int to_row) {
  int r;
  if (!vt || to_row < from_row) return;
  out_str("\0337"); /* the cursor stays where it was, as on Windows */
  for (r = from_row < 0 ? 0 : from_row; r <= to_row; r++) {
    console_goto(r, 0);
    out_str("\033[2K");
  }
  out_str("\0338");
}
