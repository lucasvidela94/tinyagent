#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "i18n.h"
#include "credential.h"
#include "login.h"
#include "settings.h"
#include "tui.h"
#include "util.h"

#define MAX_PRESETS 16
#define MAX_MODELS 300

int login_pick_model(Provider *p, const char *key) {
  char url[600], (*ids)[128], *body, msg[200];
  const char **items;
  size_t len = 0;
  int n, i, current = 0, pick;
  if (!provider_models_url(p, url, sizeof url)) {
    tui_note(tr("this provider does not publish its model list; use /model <name>"));
    return 0;
  }
  if (!(body = http_get(url, key, MAX_HTTP, &len))) return 0;
  ids = (char(*)[128])malloc(MAX_MODELS * sizeof *ids);
  items = (const char **)malloc(MAX_MODELS * sizeof *items);
  n = ids && items ? provider_parse_models(body, ids, MAX_MODELS) : 0;
  free(body);
  if (!n) tui_error(tr("the model list came back empty or in an unknown format"));
  for (i = 0; i < n; i++) {
    items[i] = ids[i];
    if (!strcmp(ids[i], p->model)) current = i;
  }
  snprintf(msg, sizeof msg, tr("Pick a model from %s (%d; type to filter, Enter):"), p->name, n);
  pick = n ? tui_select(msg, items, n, current) : -1;
  if (pick >= 0) {
    provider_set(p, "model", ids[pick]);
    tui_line(tr("model: "), p->model, COLOR_ACCENT);
  }
  free(ids);
  free(items);
  return pick >= 0;
}

int login_run(Provider *p, char *key, size_t cap) {
  const char *items[MAX_PRESETS];
  char labels[MAX_PRESETS][160], secret[1024], msg[256];
  int i, n = provider_preset_count(), pick, ok;
  Provider chosen;
  if (n > MAX_PRESETS) n = MAX_PRESETS;
  for (i = 0; i < n; i++) {
    Provider q;
    provider_preset(i, &q);
    snprintf(labels[i], sizeof labels[i], "%-14s %s", q.name, tr(provider_preset_label(i)));
    items[i] = labels[i];
  }
  if ((pick = tui_select(tr("Pick a provider (arrows and Enter):"), items, n, 0)) < 0) {
    tui_note(tr("cancelled"));
    return 0;
  }
  provider_preset(pick, &chosen);
  tui_line(tr("provider: "), chosen.name, COLOR_ACCENT);
  say(tr("\nAPI key for %s (%s): "), chosen.name, tr(CONSOLE_PASTE_HINT));
  if (read_secret(secret, sizeof secret) != LINE_OK || !*secret) {
    tui_note(tr("cancelled: no key was entered"));
    wipe(secret, sizeof secret);
    return 0;
  }
  /* Fetching the model list doubles as a key check: a rejected key is never saved. */
  say("\n");
  if (!login_pick_model(&chosen, secret)) {
    unsigned long status = http_last_status();
    if (status == 401 || status == 403) {
      tui_error(tr("the API key was rejected; nothing was saved"));
      wipe(secret, sizeof secret);
      return 0;
    }
    tui_note(tr("keeping the default model (/model to change it)"));
  }
  ok = credential_save(chosen.name, secret);
  if (ok) {
    snprintf(msg, sizeof msg, tr("key for %s saved in %s"), chosen.name, tr(CREDENTIAL_WHERE));
    tui_note(msg);
    if (!settings_save_provider(&chosen)) tui_error(tr("could not write config.ini"));
    snprintf(msg, sizeof msg, tr("done \xc2\xb7 %s \xc2\xb7 %s"), chosen.name, chosen.model);
    tui_note(msg);
    if (p) *p = chosen;
    if (key && strlen(secret) < cap) strcpy(key, secret);
  } else {
    tui_error(tr("could not save the key"));
  }
  wipe(secret, sizeof secret);
  return ok;
}
