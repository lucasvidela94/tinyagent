#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "i18n.h"
#include "tui.h"

#define VISIBLE 10

static int matches(const char *item, const char *filter) { /* case-insensitive substring */
  size_t n = strlen(filter), i, k;
  for (i = 0; item[i]; i++) {
    for (k = 0; k < n && item[i + k]; k++)
      if ((item[i + k] | 0x20) != (filter[k] | 0x20)) break;
    if (k == n) return 1;
  }
  return n == 0;
}

static int plain_select(const char *title, const char *const *items, int n, int initial) {
  char ans[128];
  int i;
  say("%s\n", title);
  for (i = 0; i < n; i++) say("  %d. %s\n", i + 1, items[i]);
  say(tr("number or name [%d]: "), initial + 1);
  if (read_line(ans, sizeof ans, 1) != LINE_OK) return -1;
  if (!*ans) return initial;
  if ((i = atoi(ans)) >= 1 && i <= n) return i - 1;
  for (i = 0; i < n; i++)
    if (!strcmp(items[i], ans)) return i;
  return -1;
}

/* Short menus (an approval, a question) show just their items; longer ones add a filter
 * hint row above and a "+N more" row below. */
#define LONG_MENU 5

/* Rows: filter hint, up to VISIBLE items, "+N more", as many as the menu needs. All
 * reserved up front so redraws never scroll. */
static int menu_rows(int n) { return (n > LONG_MENU) + (n < VISIBLE ? n : VISIBLE) + (n > VISIBLE); }

static void draw(int top, int n, const char *const *items, const int *shown, int count, int sel, int first,
                 const char *filter) {
  char hint[160];
  int i;
  console_clear(top, top + menu_rows(n) - 1);
  console_goto(top, 0);
  if (n > LONG_MENU) {
    snprintf(hint, sizeof hint, tr("  filter: %s_"), filter);
    tui_line("", *filter ? hint : tr("  (type to filter)"), COLOR_DIM);
  }
  for (i = first; i < count && i < first + VISIBLE; i++)
    tui_line(i == sel ? "> " : "  ", items[shown[i]], i == sel ? COLOR_ACCENT : COLOR_NORMAL);
  if (!count) tui_line("  ", tr("(no matches)"), COLOR_DIM);
  if (count > first + VISIBLE) {
    snprintf(hint, sizeof hint, tr("  ... +%d more"), count - first - VISIBLE);
    tui_line("", hint, COLOR_DIM);
  }
}

int tui_select(const char *title, const char *const *items, int n, int initial) {
  int *shown, count, sel, first = 0, top, i, result = -2;
  char filter[64] = "", ch;
  size_t fl = 0;
  if (n <= 0) return -1;
  if (initial < 0 || initial >= n) initial = 0;
  if (!tui_is_fancy()) return plain_select(title, items, n, initial);
  shown = (int *)malloc((size_t)n * sizeof(int));
  if (!shown) return -1;
  tui_line("", title, COLOR_DIM);
  for (i = 0; i < menu_rows(n); i++) say("\n");
  top = console_row() - menu_rows(n);
  sel = initial;
  while (result == -2) {
    for (count = 0, i = 0; i < n; i++)
      if (matches(items[i], filter)) shown[count++] = i;
    if (sel >= count) sel = count ? count - 1 : 0;
    if (sel < first) first = sel;
    if (sel >= first + VISIBLE) first = sel - VISIBLE + 1;
    draw(top, n, items, shown, count, sel, first, filter);
    switch (read_key(&ch)) {
    case KEY_UP: if (sel > 0) sel--; break;
    case KEY_DOWN: if (sel + 1 < count) sel++; break;
    case KEY_ENTER: if (count) result = shown[sel]; break;
    case KEY_ESC:
    case KEY_EOF: result = -1; break;
    case KEY_BACK: if (fl) filter[--fl] = 0; sel = first = 0; break;
    case KEY_CHAR: /* a short menu does not filter: a stray letter must not hide choices */
      if (n > LONG_MENU && fl + 1 < sizeof filter) { filter[fl++] = ch; filter[fl] = 0; }
      sel = first = 0;
      break;
    default: break;
    }
  }
  console_clear(top - 1, top + menu_rows(n) - 1); /* the title too: the caller shows the choice */
  console_goto(top - 1, 0);
  free(shown);
  return result;
}
