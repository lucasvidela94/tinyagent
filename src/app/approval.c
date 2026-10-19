#include "approval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "i18n.h"
#include "machine.h"
#include "policy.h"
#include "settings.h"
#include "shell.h"
#include "tui.h"
#include "util.h"

static int approved_always(const Session *s, const char *prefix) {
  int i;
  for (i = 0; i < s->nalways; i++)
    if (!strcmp(s->always[i], prefix)) return 1;
  return 0;
}

/* "No": the user may say what to do instead, which goes to the model with the refusal. */
static char *rejected(Session *s, int ask_why) {
  char why[MAX_LINE];
  Buf b = {0};
  s->rejected++;
  buf_str(&b, tr("The user rejected it. Nothing was run."));
  if (ask_why) {
    console_flush_input();
    say("%s", tr("  what should it do instead? (Enter to skip): "));
    if (read_line(why, sizeof why, 1) == LINE_OK && *why) {
      buf_str(&b, tr(" The user says: "));
      buf_str(&b, why);
    }
  }
  tui_panel_end(tr("not run"));
  return buf_take(&b);
}

char *approval_run(Session *s, const ToolCall *tc) {
  const Dialect *d = system_dialect();
  char rules[1024], rule[64], msg[160], prefix[64], always[128], *out;
  const char *items[3];
  int n = 0, pick, eligible = command_prefix(d, tc->command, prefix, sizeof prefix);
  Buf head = {0};
  buf_str(&head, "$ ");
  buf_str(&head, tc->command);
  tui_panel(head.p, tc->reason);
  free(head.p);
  /* Read on every command, so an edit to config.ini applies without restarting. */
  if (settings_get("deny", "commands", rules, sizeof rules) && denied_by(d, rules, tc->command, rule, sizeof rule)) {
    snprintf(msg, sizeof msg, tr("Blocked by the deny rule \"%s\" in config.ini. Nothing was run."), rule);
    tui_panel_end(msg);
    return xstrdup(msg);
  }
  s->proposed++;
  if (!eligible || !approved_always(s, prefix)) {
    items[n++] = tr("Yes, once");
    if (eligible) {
      snprintf(always, sizeof always, tr("Yes, always \"%s\" in this session"), prefix);
      items[n++] = always;
    }
    items[n++] = tr("No, and tell it what to do instead");
    console_flush_input(); /* leftover pasted lines must not answer the question */
    pick = tui_select(tr("Run it?"), items, n, 0);
    if (pick < 0) return rejected(s, 0); /* Esc */
    if (pick == n - 1) return rejected(s, 1);
    if (eligible && pick == 1 && s->nalways < MAX_ALWAYS)
      snprintf(s->always[s->nalways++], sizeof s->always[0], "%s", prefix);
  }
  s->approved++;
  out = run_command(tc->command);
  tui_output(out);
  return out;
}
