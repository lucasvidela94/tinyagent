#include "question.h"

#include "config.h"
#include "console.h"
#include "i18n.h"
#include "tui.h"

static char *answer(const char *lead, const char *text) {
  Buf b = {0};
  buf_str(&b, lead);
  buf_str(&b, text);
  return buf_take(&b);
}

char *question_ask(const ToolCall *tc) {
  const char *items[MAX_OPTIONS + 1];
  char typed[MAX_LINE];
  int i, pick;
  for (i = 0; i < tc->noptions; i++) items[i] = tc->options[i];
  items[i] = tr("Other answer (type it)");
  tui_panel(tc->question, NULL);
  console_flush_input(); /* leftover pasted lines must not answer the question */
  pick = tui_select(tr("Pick an answer (arrows and Enter, Esc to skip):"), items, tc->noptions + 1, 0);
  if (pick >= 0 && pick < tc->noptions) {
    tui_panel_end(items[pick]);
    return answer(tr("The user chose: "), items[pick]);
  }
  if (pick == tc->noptions) {
    say("%s", tr("  your answer: "));
    if (read_line(typed, sizeof typed, 1) == LINE_OK && *typed) {
      tui_panel_end(typed);
      return answer(tr("The user answered: "), typed);
    }
  }
  tui_panel_end(tr("no answer"));
  return xstrdup(tr("The user closed the question without answering."));
}
