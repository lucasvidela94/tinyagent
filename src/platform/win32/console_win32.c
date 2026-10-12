#include "win32.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "util.h"

#ifndef TMPF_TRUETYPE
#define TMPF_TRUETYPE 0x04 /* wingdi.h, excluded by WIN32_LEAN_AND_MEAN */
#endif

const char CONSOLE_PASTE_HINT[] = "clic derecho para pegar"; /* Ctrl+V does nothing in the Win7 console */

static HANDLE con_in, con_out;
static int in_is_console, out_is_console, raster_font = 1;
static WORD base_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

/* Windows 7's default console font is a raster font that only has the glyphs of the
 * console code page (CP850 in Spanish). GetCurrentConsoleFontEx is Vista+, so it is looked
 * up at run time; when unknown, assume raster (the safe choice). */
static int detect_raster_font(void) {
  typedef BOOL(WINAPI * GetFontExFn)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);
  GetFontExFn get = (GetFontExFn)(void (*)(void))GetProcAddress(GetModuleHandleW(L"kernel32.dll"),
                                                                 "GetCurrentConsoleFontEx");
  CONSOLE_FONT_INFOEX fi;
  ZeroMemory(&fi, sizeof fi);
  fi.cbSize = sizeof fi;
  if (!get || !get(con_out, FALSE, &fi)) return 1;
  return !(fi.FontFamily & TMPF_TRUETYPE);
}

void console_init(void) {
  DWORD mode;
  CONSOLE_SCREEN_BUFFER_INFO bi;
  con_in = GetStdHandle(STD_INPUT_HANDLE);
  con_out = GetStdHandle(STD_OUTPUT_HANDLE);
  in_is_console = GetConsoleMode(con_in, &mode) != 0;
  out_is_console = GetConsoleMode(con_out, &mode) != 0;
  if (out_is_console && GetConsoleScreenBufferInfo(con_out, &bi)) base_attr = bi.wAttributes;
  if (out_is_console) raster_font = detect_raster_font();
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

static int in_codepage(wchar_t c, UINT cp) {
  char tmp[8];
  BOOL used = FALSE;
  return WideCharToMultiByte(cp, WC_NO_BEST_FIT_CHARS, &c, 1, tmp, sizeof tmp, NULL, &used) > 0 && !used;
}

/* For a raster font, replaces what the code page cannot show with ascii_fallback() (or
 * '?', which is what the console would show anyway). Returns a new buffer and its length. */
static wchar_t *fit_font(const wchar_t *w, int n, int *out_n) {
  UINT cp = GetConsoleOutputCP();
  wchar_t *o = (wchar_t *)xrealloc(NULL, ((size_t)n * 3 + 1) * sizeof(wchar_t));
  int i, k = 0;
  for (i = 0; i < n; i++) {
    unsigned long c = w[i];
    const char *fb;
    if (c < 0x80 || (c < 0xD800 && in_codepage(w[i], cp)) || (c >= 0xE000 && in_codepage(w[i], cp))) {
      o[k++] = w[i];
      continue;
    }
    if (c >= 0xD800 && c <= 0xDBFF && i + 1 < n && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF)
      c = 0x10000 + ((c - 0xD800) << 10) + (w[++i] - 0xDC00);
    if ((fb = ascii_fallback(c)) != NULL)
      while (*fb) o[k++] = (wchar_t)*fb++;
    else
      o[k++] = L'?';
  }
  *out_n = k;
  return o;
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
  if (raster_font) {
    wchar_t *fitted = fit_font(w, n, &n);
    free(w);
    w = fitted;
  }
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
