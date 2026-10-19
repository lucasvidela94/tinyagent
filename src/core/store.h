/* Saved conversations: one SQLite database (vendor/sqlite) in the data folder, with the
 * sessions and their messages as the JSON the API takes. Portable C, the only module that
 * includes sqlite3.h (docs/adr/0002-sqlite-session-store.md). Every write is its own
 * transaction, so a power cut loses at most the message being written. */
#ifndef TINYSHELL_STORE_H
#define TINYSHELL_STORE_H

#include "conversation.h"

typedef struct Store Store;

typedef struct {
  long id;
  char when[20];   /* "YYYY-MM-DD HH:MM" of the last message, UTC */
  char title[96];  /* the first user message, cut to fit */
  int messages;
} StoreItem;

Store *store_open(const char *path); /* creates the tables if needed; NULL on failure */
void store_close(Store *st);

long store_new_session(Store *st, const char *model); /* its id, or 0 on failure */
/* Appends one message (the history's JSON). The first user message, as text, becomes
 * the session's title. Returns 1 on success. */
int store_add(Store *st, long session, const char *json, int user, const char *text);

/* The most recently used sessions, newest first, at most `max`. With a non-empty `filter`,
 * only sessions whose title or messages contain it (ASCII letters ignore case). Returns
 * how many were written to `out`. */
int store_list(Store *st, const char *filter, StoreItem *out, int max);

/* Appends a session's messages to `h` (which keeps its own system prompt). Returns how
 * many were added, or -1 if the session does not exist. */
int store_load(Store *st, long session, History *h);

/* Deletes all but the `keep` most recently used sessions. */
void store_prune(Store *st, int keep);

#endif
