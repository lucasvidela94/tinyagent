#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "i18n.h"
#include "tui.h"
#include "util.h"

#define PREVIEW_LINES 8
#define RULE "\xe2\x94\x80" /* U+2500, present in the OEM raster fonts */

static int fancy, thinking_row = -1;

void tui_init(void) { fancy = console_is_tty() && console_width() >= 40; }

int tui_is_fancy(void) { return fancy; }

static void rule(void) {
  Buf b = {0};
  int i, w = console_width();
  for (i = 0; i < w - 1; i++) buf_str(&b, RULE);
  buf_str(&b, "\n");
  console_write(b.p, COLOR_DIM);
  free(b.p);
}

/* Full-width rows in `color`, wrapped at spaces. Each row is exactly as wide as the
 * buffer, so the background covers it and the console moves to the next row itself. */
static void block(const char *text, int color) {
  int w = console_width();
  const char *p = text;
  do {
    size_t n = text_fit(p, w - 2);
    int cols = 1 + utf8_cols(p, n);
    Buf b = {0};
    buf_str(&b, " ");
    buf_add(&b, p, n);
    for (; cols < w; cols++) buf_str(&b, " ");
    console_write(b.p, color);
    free(b.p);
    p += n;
    if (*p == '\n') p++;
    else while (*p == ' ') p++;
  } while (*p);
}

static void footer(const char *left, const char *right) {
  int w = console_width() - 1, lc = utf8_cols(left, strlen(left)), rc = utf8_cols(right, strlen(right));
  Buf b = {0};
  if (lc + 1 + rc > w) { /* keep the end of the left part (the deepest folder) */
    const char *p = left;
    while (*p && lc + 4 + rc > w) {
      int n = utf8_len((const unsigned char *)p);
      p += n ? n : 1;
      lc--;
    }
    buf_str(&b, "...");
    buf_str(&b, p);
    lc += 3;
  } else {
    buf_str(&b, left);
  }
  for (; lc + rc < w; lc++) buf_str(&b, " ");
  buf_str(&b, right);
  console_write(b.p, COLOR_DIM);
  free(b.p);
}

int tui_prompt(char *out, size_t cap, const char *left, const char *right) {
  int r, bottom, cur;
  if (!fancy) {
    say("\n> ");
    return read_line(out, cap, 1);
  }
  /* Reserve the four rows first: if drawing scrolled the buffer, the remembered rows
   * would point at the wrong lines. */
  say("\n\n\n\n\n");
  bottom = console_row() - 1; /* footer row; above it: rule, editor, rule */
  console_goto(bottom - 3, 0);
  rule();
  say(">\n");
  rule();
  footer(left, right);
  console_goto(bottom - 2, 2);
  r = read_line(out, cap, 1);
  cur = console_row();
  console_clear(bottom - 3, cur > bottom ? cur : bottom);
  console_goto(bottom - 3, 0);
  return r;
}

void tui_user(const char *text) {
  if (fancy) block(text, COLOR_USER);
}

void tui_thinking(int on) {
  if (!fancy) return;
  if (on) {
    say("\n");
    thinking_row = console_row();
    console_write(tr("thinking..."), COLOR_DIM);
  } else if (thinking_row >= 0) {
    int printed = console_row() != thinking_row; /* e.g. a network error below it */
    console_clear(thinking_row, thinking_row);
    if (!printed) console_goto(thinking_row - 1, 0);
    thinking_row = -1;
  }
}

void tui_text(const char *text) { say("\n%s\n", text); }

static int streaming;

void tui_stream(const char *text) {
  if (!streaming) {
    say("\n");
    streaming = 1;
  }
  console_write(text, COLOR_NORMAL);
}

void tui_stream_end(void) {
  if (streaming) say("\n");
  streaming = 0;
}

void tui_line(const char *indent, const char *text, int color) {
  console_write(indent, color);
  console_write(text, color);
  say("\n");
}

void tui_note(const char *text) { tui_line("", text, COLOR_DIM); }
void tui_error(const char *text) { tui_line("", text, COLOR_DANGER); }

void tui_proposal(const char *cmd, const char *reason, const char *risk) {
  say("\n");
  tui_line("$ ", cmd, risk ? COLOR_DANGER : COLOR_ACCENT);
  tui_line("  ", *reason ? reason : tr("(no reason given)"), COLOR_DIM);
  if (risk) tui_line(tr("  risk: "), risk, COLOR_DANGER);
}

/* Where the notes command_result() appends begin: the trailing "[...]" lines, in whatever
 * language (exit code, truncation, timeout, Ctrl+C). */
static const char *notes_start(const char *s) {
  const char *end = s + strlen(s), *p = end, *start = end;
  while (p > s) {
    const char *line = p, *stop = p;
    while (stop > s && (stop[-1] == '\n' || stop[-1] == '\r')) stop--;
    for (line = stop; line > s && line[-1] != '\n'; line--) {}
    if (stop == line) { /* blank line */
      p = line;
      continue;
    }
    if (line[0] != '[' || stop[-1] != ']') break;
    start = p = line;
  }
  return start;
}

void tui_output(const char *output) {
  char *copy = xstrdup(output), *l, *next, *notes = copy + (notes_start(output) - output);
  int shown = 0, hidden = 0, w = fancy ? console_width() - 3 : 1 << 20;
  Buf meta = {0};
  for (l = copy; l; l = next) {
    size_t n;
    next = strchr(l, '\n');
    if (next) *next++ = 0;
    n = strlen(l);
    if (n && l[n - 1] == '\r') l[--n] = 0;
    if (!l[strspn(l, " \t")]) continue; /* blank lines add nothing to a preview */
    if (l >= notes) {
      buf_str(&meta, meta.len ? " · " : "");
      buf_add(&meta, l + 1, n > 1 ? n - 2 : 0); /* without the brackets */
    } else if (shown < PREVIEW_LINES) {
      l[text_fit(l, w)] = 0;
      tui_line("  ", l, COLOR_DIM);
      shown++;
    } else {
      hidden++;
    }
  }
  if (hidden || meta.len) {
    Buf b = {0};
    char num[48];
    if (hidden) {
      snprintf(num, sizeof num, tr("... +%d lines"), hidden);
      buf_str(&b, num);
    }
    if (meta.len) {
      buf_str(&b, hidden ? " · " : "");
      buf_str(&b, meta.p);
    }
    tui_line("  ", b.p, COLOR_DIM);
    free(b.p);
  }
  free(meta.p);
  free(copy);
}
