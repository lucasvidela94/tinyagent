#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "i18n.h"
#include "login.h"
#include "reporter.h"
#include "settings.h"
#include "slash.h"
#include "tui.h"
#include "update.h"
#include "util.h"

/* 1 if input is the command (alone, or followed by a space and an argument). */
static int is(const char *input, const char *name, const char *alias) {
  size_t n = strlen(name), a = alias ? strlen(alias) : 0;
  return (!strncmp(input, name, n) && (!input[n] || input[n] == ' ')) ||
         (alias && !strncmp(input, alias, a) && (!input[a] || input[a] == ' '));
}

static const char *argument(const char *input) {
  const char *p = strchr(input, ' ');
  if (!p) return "";
  while (*p == ' ') p++;
  return p;
}

/* "/model" lists the provider's models to pick from; "/model X" sets one directly. Either
 * way the choice is saved to config.ini. */
static void model_command(Session *s, const char *arg) {
  Buf msg = {0};
  const char *err;
  if (*arg && (err = provider_set(&s->provider, "model", arg)) != NULL) {
    tui_error(tr(err));
    return;
  }
  if ((*arg || login_pick_model(&s->provider, s->api_key)) && !settings_save_model(s->provider.model))
    tui_error(tr("could not save to config.ini (it applies to this session)"));
  buf_str(&msg, s->provider.name);
  buf_str(&msg, " \xc2\xb7 ");
  buf_str(&msg, s->provider.model);
  buf_str(&msg, "\n");
  buf_str(&msg, s->provider.url);
  buf_str(&msg, "\n");
  buf_str(&msg, tr("change it in "));
  buf_str(&msg, settings_path());
  tui_note(msg.p);
  free(msg.p);
}

static void help(void) {
  tui_note(tr("/model [x]  pick a model from the list (or type its name)\n"
              "/login      change provider or API key\n"
              "/report     tell the developers about a problem\n"
              "/reset      start a new conversation\n"
              "/update     check for and install the latest version\n"
              "/quit       quit"));
}

int slash_run(Session *s, const char *input) {
  if (input[0] != '/') return SLASH_NONE;
  if (is(input, "/quit", "/salir") || is(input, "/exit", NULL)) return SLASH_QUIT;
  if (is(input, "/help", "/ayuda")) help();
  else if (is(input, "/reset", "/reiniciar")) {
    session_reset(s);
    tui_note(tr("new conversation"));
  } else if (is(input, "/update", "/actualizar")) update_run();
  else if (is(input, "/report", "/reporte")) reporter_run(s);
  else if (is(input, "/login", "/clave")) login_run(&s->provider, s->api_key, sizeof s->api_key);
  else if (is(input, "/model", "/modelo")) model_command(s, argument(input));
  else tui_error(tr("Unknown command. /help lists the commands."));
  return SLASH_DONE;
}
