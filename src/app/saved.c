#include "saved.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i18n.h"
#include "settings.h"
#include "store.h"
#include "sys.h"
#include "tui.h"
#include "util.h"

#define LISTED 20 /* sessions shown by /sessions */

void saved_open(Session *s) {
  char v[8];
  Buf path = {0};
  StoreItem last;
  s->store = NULL;
  s->store_id = 0;
  if (settings_get("sessions", "save", v, sizeof v) && atoi(v) == 0) return;
  if (sys_data_path("sessions.db", &path)) s->store = store_open(path.p);
  free(path.p);
  if (!s->store) {
    tui_note(tr("conversations will not be saved (the sessions file could not be opened)"));
    return;
  }
  store_prune(s->store, SAVED_KEEP);
  if (store_list(s->store, "", &last, 1)) tui_note(tr("/sessions resumes a saved conversation"));
}

void saved_close(Session *s) {
  store_close(s->store);
  s->store = NULL;
}

void saved_add(Session *s, char *json, int user, const char *text) {
  if (s->store) {
    if (!s->store_id) s->store_id = store_new_session(s->store, s->provider.model);
    if (s->store_id) store_add(s->store, s->store_id, json, user, text);
  }
  history_add(&s->history, json, user);
}

void saved_pick(Session *s, const char *filter) {
  StoreItem items[LISTED];
  char lines[LISTED][160], msg[192];
  const char *labels[LISTED];
  int n, i, pick, loaded;
  if (!s->store) {
    tui_note(tr("saving conversations is off ([sessions] save = 0 in config.ini)"));
    return;
  }
  if (!(n = store_list(s->store, filter, items, LISTED))) {
    tui_note(*filter ? tr("no saved conversation contains that") : tr("no saved conversations yet"));
    return;
  }
  for (i = 0; i < n; i++) {
    snprintf(lines[i], sizeof lines[i], "%s  %s  (%d)", items[i].when, *items[i].title ? items[i].title : "-",
             items[i].messages);
    labels[i] = lines[i];
  }
  if ((pick = tui_select(tr("Pick a conversation to resume:"), labels, n, 0)) < 0) return;
  session_reset(s); /* the system prompt stays: it describes this machine, now */
  loaded = store_load(s->store, items[pick].id, &s->history);
  if (loaded < 0) {
    tui_error(tr("That conversation could not be loaded."));
    return;
  }
  s->store_id = items[pick].id; /* new messages continue it */
  snprintf(msg, sizeof msg, tr("resumed: %s (%d messages)"), items[pick].title, loaded);
  tui_note(msg);
}
