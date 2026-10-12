/* TinyShell: general-purpose terminal agent for old PCs. Entry point: CLI flags,
 * API key setup and the chat loop with its slash commands. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "config.h"
#include "console.h"
#include "cli.h"
#include "credential.h"
#include "i18n.h"
#include "machine.h"
#include "policy.h"
#include "session.h"
#include "settings.h"
#include "slash.h"
#include "sys.h"
#include "telemetry.h"
#include "tui.h"
#include "util.h"

/* The interface language: [ui] language in config.ini (es, en or auto), else the system's. */
static void language_init(void) {
  char v[16];
  int ok, lang = i18n_parse(system_language(), &ok), forced;
  if (settings_get("ui", "language", v, sizeof v)) {
    forced = i18n_parse(v, &ok);
    if (ok) lang = forced;
  }
  i18n_set(lang);
}

/* Asks for the key (hidden) and offers to save it. Empty is allowed (local servers
 * usually need none). 0 only on EOF. */
static int ask_key(const Provider *p, char *key, size_t cap) {
  char answer[16], msg[160];
  say(tr("API key for %s (%s; Enter if none): "), p->name, tr(CONSOLE_PASTE_HINT));
  if (read_secret(key, cap) != LINE_OK) return 0;
  if (!*key) {
    tui_note(tr("no API key"));
    return 1;
  }
  snprintf(msg, sizeof msg, tr("key received (%d characters)"), (int)strlen(key));
  tui_note(msg);
  snprintf(msg, sizeof msg, tr("Save it in %s? [y/N] "), tr(CREDENTIAL_WHERE));
  say("%s", msg);
  if (read_line(answer, sizeof answer, 1) == LINE_OK && is_yes(answer))
    tui_note(credential_save(p->name, key) ? tr("saved \xc2\xb7 tinyshell logout deletes it") : tr("could not save it"));
  return 1;
}

static void fmt_tokens(char *out, size_t cap, long n) {
  if (n < 1000) snprintf(out, cap, "%ld", n);
  else snprintf(out, cap, "%ld.%ldk", n / 1000, (n % 1000) / 100);
}

static void status_left(const Session *s, char *out, size_t cap) {
  Buf cwd = {0};
  char in[16], o[16];
  int pct = (int)(s->history.bytes * 100 / MAX_HISTORY);
  sys_cwd(&cwd);
  fmt_tokens(in, sizeof in, s->tokens_in);
  fmt_tokens(o, sizeof o, s->tokens_out);
  snprintf(out, cap, tr("%s \xc2\xb7 %s in \xc2\xb7 %s out \xc2\xb7 %d%%"), cwd.p ? cwd.p : "", in, o, pct > 100 ? 100 : pct);
  free(cwd.p);
}

static void chat(Session *s) {
  char input[MAX_LINE], left[1024];
  for (;;) {
    int r, cmd;
    unsigned long status;
    status_left(s, left, sizeof left);
    r = tui_prompt(input, sizeof input, left, s->provider.model);
    if (r == LINE_EOF) return;
    if (r == LINE_OK && *input) tui_user(input);
    if (r == LINE_TOO_LONG) {
      tui_error(tr("Message too long; it was not sent."));
      continue;
    }
    if (!*input) continue;
    if ((cmd = slash_run(s, input)) == SLASH_QUIT) return;
    if (cmd == SLASH_DONE) continue;
    if ((status = session_send(s, input)) == 401 || status == 403)
      tui_error(tr("The API key was rejected. /login to change it."));
  }
}

int main(int argc, char **argv) {
  Session s;
  char banner[160];
  console_init();
  cancel_init();
  tui_init();
  language_init();
  if (argc > 1) return cli_run(argc, argv);
  settings_load(&s.provider);
  snprintf(banner, sizeof banner, "TinyShell %s \xc2\xb7 %s \xc2\xb7 /help", TINYSHELL_VERSION, s.provider.name);
  tui_note(banner);
  if (provider_insecure(&s.provider))
    tui_error(tr("warning: the URL is http://, the API key and the conversation travel unencrypted"));
  if (credential_load(s.provider.name, s.api_key, sizeof s.api_key))
    tui_note(tr("using the saved API key \xc2\xb7 /login to change it"));
  else if (!ask_key(&s.provider, s.api_key, sizeof s.api_key)) {
    tui_error(tr("The API key is missing."));
    wipe(s.api_key, sizeof s.api_key);
    return 1;
  }
  telemetry_init();
  session_init(&s);
  {
    char data[400];
    snprintf(data, sizeof data, "{\"provider\":\"%s\",\"model\":\"%s\",\"tui\":%d}", s.provider.name,
             s.provider.model, tui_is_fancy());
    telemetry_event("session_start", data);
  }
  chat(&s);
  {
    char data[300];
    snprintf(data, sizeof data,
             "{\"turns\":%d,\"proposed\":%d,\"approved\":%d,\"rejected\":%d,\"tokens_in\":%ld,"
             "\"tokens_out\":%ld,\"seconds\":%lu}",
             s.turns, s.proposed, s.approved, s.rejected, s.tokens_in, s.tokens_out,
             (sys_ms() - s.started_ms) / 1000);
    telemetry_event("session_end", data);
  }
  session_free(&s);
  return 0;
}
