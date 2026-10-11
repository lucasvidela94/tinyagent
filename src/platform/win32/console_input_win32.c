/* Console input (second half of the console module; output is in console_win32.c).
 * Reads through the wide API so non-ASCII input works on the Windows 7 console. */
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "console.h"
#include "util.h"

static HANDLE con_in;
static int in_is_console = -1;

static void input_init(void) {
  DWORD mode;
  if (in_is_console >= 0) return;
  con_in = GetStdHandle(STD_INPUT_HANDLE);
  in_is_console = GetConsoleMode(con_in, &mode) != 0;
}

void console_flush_input(void) {
  input_init();
  if (in_is_console) FlushConsoleInputBuffer(con_in);
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

int read_line(char *out, size_t cap, int echo) {
  int status = LINE_OK, got_any = 0, eol = 0, n;
  size_t wn = 0;
  DWORD mode = 0;
  wchar_t *w;
  out[0] = 0;
  input_init();
  if (!in_is_console) return read_redirected(out, cap);
  GetConsoleMode(con_in, &mode);
  if (!echo) SetConsoleMode(con_in, mode & ~(DWORD)ENABLE_ECHO_INPUT);
  w = (wchar_t *)xrealloc(NULL, cap * sizeof(wchar_t));
  while (!eol) {
    wchar_t chunk[256];
    DWORD r = 0, k;
    if (!ReadConsoleW(con_in, chunk, 256, &r, NULL) || r == 0) break;
    if (!got_any && chunk[0] == 0x1A) break; /* Ctrl+Z */
    got_any = 1;
    for (k = 0; k < r; k++) {
      if (chunk[k] == L'\n') { eol = 1; break; }
      if (chunk[k] == L'\r') continue;
      if (wn < cap - 1) w[wn++] = chunk[k];
      else status = LINE_TOO_LONG;
    }
  }
  if (!echo) {
    SetConsoleMode(con_in, mode);
    say("\n");
  }
  n = wn ? WideCharToMultiByte(CP_UTF8, 0, w, (int)wn, out, (int)cap - 1, NULL, NULL) : 0;
  if (wn && n == 0) status = LINE_TOO_LONG; /* UTF-8 form does not fit */
  out[n] = 0;
  SecureZeroMemory(w, cap * sizeof(wchar_t)); /* may hold the API key */
  free(w);
  trim_line(out);
  if (cancel_take_input_interrupt()) { /* Ctrl+C: the line is discarded */
    out[0] = 0;
    return LINE_EOF;
  }
  return got_any ? status : LINE_EOF;
}

/* Wide chars to UTF-8 in out; LINE_TOO_LONG if they do not fit. Wipes w. */
static int finish_wide(wchar_t *w, size_t wn, size_t wcap, char *out, size_t cap, int status) {
  int n = wn ? WideCharToMultiByte(CP_UTF8, 0, w, (int)wn, out, (int)cap - 1, NULL, NULL) : 0;
  if (wn && n == 0) status = LINE_TOO_LONG;
  out[n] = 0;
  SecureZeroMemory(w, wcap * sizeof(wchar_t));
  free(w);
  return status;
}

int read_secret(char *out, size_t cap) {
  DWORD mode = 0, r;
  wchar_t c, *w;
  size_t wn = 0;
  int status = LINE_OK, got_any = 0;
  out[0] = 0;
  input_init();
  if (!in_is_console) return read_redirected(out, cap);
  GetConsoleMode(con_in, &mode);
  /* Character mode: each key (or pasted character) arrives on its own, unechoed. */
  SetConsoleMode(con_in, mode & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
  w = (wchar_t *)xrealloc(NULL, cap * sizeof(wchar_t));
  for (;;) {
    if (!ReadConsoleW(con_in, &c, 1, &r, NULL) || r == 0) break;
    got_any = 1;
    if (c == L'\r' || c == L'\n') break;
    if (c == 0x1A && wn == 0) { got_any = 0; break; } /* Ctrl+Z */
    if (c == 8) {
      if (wn) {
        wn--;
        say("\b \b");
      }
      continue;
    }
    if (c < 0x20) continue;
    if (wn < cap - 1) {
      w[wn++] = c;
      say("*");
    } else {
      status = LINE_TOO_LONG;
    }
  }
  SetConsoleMode(con_in, mode);
  say("\n");
  status = finish_wide(w, wn, cap, out, cap, status);
  trim_line(out);
  if (cancel_take_input_interrupt()) {
    SecureZeroMemory(out, cap);
    return LINE_EOF;
  }
  return got_any ? status : LINE_EOF;
}

int read_key(char *ch) {
  INPUT_RECORD ev;
  DWORD r;
  input_init();
  if (!in_is_console) return KEY_OTHER;
  for (;;) {
    if (!ReadConsoleInputW(con_in, &ev, 1, &r) || r == 0) return KEY_EOF;
    if (ev.EventType != KEY_EVENT || !ev.Event.KeyEvent.bKeyDown) continue;
    switch (ev.Event.KeyEvent.wVirtualKeyCode) {
    case VK_UP: return KEY_UP;
    case VK_DOWN: return KEY_DOWN;
    case VK_RETURN: return KEY_ENTER;
    case VK_ESCAPE: return KEY_ESC;
    case VK_BACK: return KEY_BACK;
    default:
      if (ev.Event.KeyEvent.uChar.UnicodeChar >= 0x20 && ev.Event.KeyEvent.uChar.UnicodeChar < 0x7F) {
        *ch = (char)ev.Event.KeyEvent.uChar.UnicodeChar;
        return KEY_CHAR;
      }
      break;
    }
  }
}
