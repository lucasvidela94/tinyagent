#include "win32.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "util.h"

static HANDLE con_in, con_out;
static int in_is_console, out_is_console;
static WORD base_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

void console_init(void) {
  DWORD mode;
  CONSOLE_SCREEN_BUFFER_INFO bi;
  con_in = GetStdHandle(STD_INPUT_HANDLE);
  con_out = GetStdHandle(STD_OUTPUT_HANDLE);
  in_is_console = GetConsoleMode(con_in, &mode) != 0;
  out_is_console = GetConsoleMode(con_out, &mode) != 0;
  if (out_is_console && GetConsoleScreenBufferInfo(con_out, &bi)) base_attr = bi.wAttributes;
}

/* 16-color console attributes; the user's background is kept except for the user block. */
static WORD attr_for(int color) {
  WORD bg = base_attr & 0xF0;
  switch (color) {
  case COLOR_DIM: return bg | FOREGROUND_INTENSITY;
  case COLOR_ACCENT: return bg | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
  case COLOR_DANGER: return bg | FOREGROUND_RED | FOREGROUND_INTENSITY;
  case COLOR_USER: return BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
  default: return base_attr;
  }
}

int console_is_tty(void) { return in_is_console && out_is_console; }

static int buffer_info(CONSOLE_SCREEN_BUFFER_INFO *bi) {
  return out_is_console && GetConsoleScreenBufferInfo(con_out, bi);
}

int console_width(void) {
  CONSOLE_SCREEN_BUFFER_INFO bi;
  return buffer_info(&bi) ? bi.dwSize.X : 80;
}

int console_row(void) {
  CONSOLE_SCREEN_BUFFER_INFO bi;
  return buffer_info(&bi) ? bi.dwCursorPosition.Y : 0;
}

void console_goto(int row, int col) {
  COORD c;
  c.X = (SHORT)col;
  c.Y = (SHORT)(row < 0 ? 0 : row);
  if (out_is_console) SetConsoleCursorPosition(con_out, c);
}

void console_clear(int from_row, int to_row) {
  DWORD done, len;
  COORD c;
  if (!out_is_console || to_row < from_row) return;
  c.X = 0;
  c.Y = (SHORT)(from_row < 0 ? 0 : from_row);
  len = (DWORD)(to_row - c.Y + 1) * (DWORD)console_width();
  FillConsoleOutputCharacterW(con_out, L' ', len, c, &done);
  FillConsoleOutputAttribute(con_out, base_attr, len, c, &done);
}

void console_flush_input(void) {
  if (in_is_console) FlushConsoleInputBuffer(con_in);
}

static void write_utf8(const char *s) {
  int n, off = 0;
  wchar_t *w;
  if (!out_is_console) { /* redirected: write UTF-8 bytes as they are */
    fputs(s, stdout);
    fflush(stdout);
    return;
  }
  n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (n <= 1) return;
  w = (wchar_t *)xrealloc(NULL, (size_t)n * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
  n--;
  while (off < n) { /* conhost on Windows 7 rejects very large single writes */
    DWORD done = 0;
    int chunk = n - off > 8192 ? 8192 : n - off;
    if (chunk < n - off && w[off + chunk - 1] >= 0xD800 && w[off + chunk - 1] <= 0xDBFF) chunk--;
    if (!WriteConsoleW(con_out, w + off, (DWORD)chunk, &done, NULL)) break;
    off += chunk;
  }
  free(w);
}

void console_write(const char *s, int color) {
  int colored = out_is_console && color != COLOR_NORMAL;
  if (colored) SetConsoleTextAttribute(con_out, attr_for(color));
  write_utf8(s);
  if (colored) SetConsoleTextAttribute(con_out, base_attr);
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
    write_utf8(s);
    free(s);
  }
  va_end(ap2);
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
    write_utf8("\n");
  }
  n = wn ? WideCharToMultiByte(CP_UTF8, 0, w, (int)wn, out, (int)cap - 1, NULL, NULL) : 0;
  if (wn && n == 0) status = LINE_TOO_LONG; /* UTF-8 form does not fit */
  out[n] = 0;
  SecureZeroMemory(w, cap * sizeof(wchar_t)); /* may hold the API key */
  free(w);
  trim_line(out);
  return got_any ? status : LINE_EOF;
}
