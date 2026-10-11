/* Console input (second half of the console module; output is in console_posix.c).
 * Lines use the terminal's own line editing (canonical mode); secrets and menu keys switch
 * to character mode only for the duration of the call. Ctrl+C stays a signal: read()
 * returns EINTR and the reader reports EOF. */
#include "posix.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "cancel.h"
#include "charset.h"
#include "console.h"
#include "console_posix.h"
#include "util.h"

static int in_tty(void) { return isatty(0); }

void console_flush_input(void) {
  if (in_tty()) tcflush(0, TCIFLUSH);
}

static int read_redirected(char *out, size_t cap) {
  size_t len;
  int c, status = LINE_OK;
  if (!fgets(out, (int)cap, stdin)) return LINE_EOF;
  len = strlen(out);
  if (len && out[len - 1] != '\n' && !feof(stdin)) {
    while ((c = getchar()) != EOF && c != '\n') {}
    status = LINE_TOO_LONG;
  }
  trim_line(out);
  return status;
}

/* Terminal bytes to UTF-8 in out (cap bytes); LINE_TOO_LONG if they do not fit. Wipes raw. */
static int finish(Buf *raw, char *out, size_t cap, int status) {
  Buf u = {0};
  charset_decode(raw->p ? raw->p : "", raw->len, console_charset(), &u);
  if (u.len >= cap) status = LINE_TOO_LONG;
  else memcpy(out, u.p ? u.p : "", u.len + 1);
  if (u.p) wipe(u.p, u.len); /* may hold the API key */
  if (raw->p) wipe(raw->p, raw->len);
  free(u.p);
  free(raw->p);
  trim_line(out);
  return status;
}

/* Character or line mode for one call; restored by mode_restore. */
static void mode_set(struct termios *saved, int line, int echo) {
  struct termios t;
  tcgetattr(0, saved);
  t = *saved;
  if (line) t.c_lflag |= ICANON;
  else t.c_lflag &= ~(tcflag_t)ICANON;
  if (echo) t.c_lflag |= ECHO;
  else t.c_lflag &= ~(tcflag_t)ECHO;
#ifdef IUTF8
  if (console_charset() == CHARSET_UTF8) t.c_iflag |= IUTF8; /* Backspace erases a whole character */
#endif
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &t);
}

static void mode_restore(const struct termios *saved) { tcsetattr(0, TCSANOW, saved); }

/* 1 byte, 0 on EOF, -1 if Ctrl+C interrupted the read. */
static int get_byte(char *c) {
  for (;;) {
    ssize_t r = read(0, c, 1);
    if (r == 1) return 1;
    if (r == 0) return 0;
    if (errno != EINTR) return 0;
    if (cancel_take_input_interrupt()) return -1;
  }
}

int read_line(char *out, size_t cap, int echo) {
  struct termios saved;
  Buf raw = {0};
  int status = LINE_OK, got = 0, r = 0;
  char c;
  out[0] = 0;
  if (!in_tty()) return read_redirected(out, cap);
  mode_set(&saved, 1, echo);
  while ((r = get_byte(&c)) == 1) {
    got = 1;
    if (c == '\n') break;
    if (raw.len < cap - 1) buf_add(&raw, &c, 1);
    else status = LINE_TOO_LONG; /* keep consuming: the rest must not reach the next prompt */
  }
  mode_restore(&saved);
  if (!echo) say("\n");
  status = finish(&raw, out, cap, status);
  if (r < 0) { /* Ctrl+C: the line is discarded */
    wipe(out, cap);
    out[0] = 0;
    return LINE_EOF;
  }
  return got ? status : LINE_EOF;
}

int read_secret(char *out, size_t cap) {
  struct termios saved;
  Buf raw = {0};
  int status = LINE_OK, got = 0, r = 0;
  char c;
  out[0] = 0;
  if (!in_tty()) return read_redirected(out, cap);
  mode_set(&saved, 0, 0);
  while ((r = get_byte(&c)) == 1) {
    got = 1;
    if (c == '\r' || c == '\n') break;
    if (c == 4 && raw.len == 0) { got = 0; break; } /* Ctrl+D */
    if (c == 0x7F || c == 8) {
      while (raw.len && (raw.p[raw.len - 1] & 0xC0) == 0x80) raw.len--; /* UTF-8 continuation */
      if (raw.len) {
        raw.len--;
        say("\b \b");
      }
      continue;
    }
    if ((unsigned char)c < 0x20) continue;
    if (raw.len < cap - 1) {
      buf_add(&raw, &c, 1);
      if ((c & 0xC0) != 0x80) say("*"); /* one per character, not per byte */
    } else {
      status = LINE_TOO_LONG;
    }
  }
  mode_restore(&saved);
  say("\n");
  status = finish(&raw, out, cap, status);
  if (r < 0) {
    wipe(out, cap);
    return LINE_EOF;
  }
  return got ? status : LINE_EOF;
}

/* After ESC: "[A"/"OA" (arrows, normal or application mode) arrive within a moment; a
 * lone ESC does not. */
static int escape_key(void) {
  struct termios t, saved;
  char seq[2];
  ssize_t n;
  tcgetattr(0, &saved);
  t = saved;
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 1;
  tcsetattr(0, TCSANOW, &t);
  n = read(0, seq, 1);
  if (n == 1 && (seq[0] == '[' || seq[0] == 'O')) n += read(0, seq + 1, 1);
  tcsetattr(0, TCSANOW, &saved);
  if (n < 1) return KEY_ESC;
  if (n == 2 && seq[1] == 'A') return KEY_UP;
  if (n == 2 && seq[1] == 'B') return KEY_DOWN;
  return KEY_OTHER;
}

int read_key(char *ch) {
  struct termios saved;
  int key = KEY_OTHER, r;
  char c;
  if (!in_tty()) return KEY_OTHER;
  mode_set(&saved, 0, 0);
  while (key == KEY_OTHER) { /* other keys and unknown sequences: wait for the next */
    if ((r = get_byte(&c)) != 1) {
      key = KEY_EOF;
      break;
    }
    if (c == '\r' || c == '\n') key = KEY_ENTER;
    else if (c == 0x7F || c == 8) key = KEY_BACK;
    else if (c == '\033') key = escape_key();
    else if (c >= 0x20 && c < 0x7F) {
      *ch = c;
      key = KEY_CHAR;
    }
  }
  mode_restore(&saved);
  return key;
}
