#include "store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include "util.h"

struct Store {
  sqlite3 *db;
};

/* updated: when the last message arrived, to sort and prune by use, not by creation. */
static const char SCHEMA[] =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS sessions (id INTEGER PRIMARY KEY, model TEXT NOT NULL,"
    "  title TEXT NOT NULL DEFAULT '', started TEXT NOT NULL, updated TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY,"
    "  session INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
    "  user INTEGER NOT NULL, json TEXT NOT NULL);"
    "CREATE INDEX IF NOT EXISTS messages_session ON messages (session, id);";

Store *store_open(const char *path) {
  Store *st = (Store *)calloc(1, sizeof *st);
  if (!st) return NULL;
  if (sqlite3_open(path, &st->db) != SQLITE_OK || sqlite3_exec(st->db, SCHEMA, NULL, NULL, NULL) != SQLITE_OK) {
    store_close(st);
    return NULL;
  }
  sqlite3_busy_timeout(st->db, 2000); /* two TinyShell windows may share the file */
  return st;
}

void store_close(Store *st) {
  if (!st) return;
  sqlite3_close(st->db);
  free(st);
}

static sqlite3_stmt *prepare(Store *st, const char *sql) {
  sqlite3_stmt *s = NULL;
  if (sqlite3_prepare_v2(st->db, sql, -1, &s, NULL) != SQLITE_OK) return NULL;
  return s;
}

/* Steps a statement that returns no rows and frees it; 1 on success. */
static int run(sqlite3_stmt *s) {
  int ok = s && sqlite3_step(s) == SQLITE_DONE;
  sqlite3_finalize(s);
  return ok;
}

long store_new_session(Store *st, const char *model) {
  sqlite3_stmt *s = prepare(st, "INSERT INTO sessions (model, started, updated) VALUES (?, datetime('now'), "
                                "datetime('now'))");
  if (!s) return 0;
  sqlite3_bind_text(s, 1, model, -1, SQLITE_TRANSIENT);
  return run(s) ? (long)sqlite3_last_insert_rowid(st->db) : 0;
}

int store_add(Store *st, long session, const char *json, int user, const char *text) {
  sqlite3_stmt *s = prepare(st, "INSERT INTO messages (session, user, json) VALUES (?, ?, ?)");
  char title[sizeof(((StoreItem *)0)->title)];
  if (!s) return 0;
  sqlite3_bind_int64(s, 1, session);
  sqlite3_bind_int(s, 2, user != 0);
  sqlite3_bind_text(s, 3, json, -1, SQLITE_TRANSIENT);
  if (!run(s)) return 0;
  s = prepare(st, "UPDATE sessions SET updated = datetime('now'), title = CASE WHEN title = '' AND ?2 <> '' "
                  "THEN ?2 ELSE title END WHERE id = ?1");
  if (!s) return 0;
  title[0] = 0;
  if (user && text) { /* one line, cut where it fits */
    size_t n = text_fit(text, (int)(sizeof title - 1) / 3), i; /* a column is at most 3 bytes */
    memcpy(title, text, n);
    title[n] = 0;
    for (i = 0; i < n; i++)
      if (title[i] == '\n' || title[i] == '\r' || title[i] == '\t') title[i] = ' ';
  }
  sqlite3_bind_int64(s, 1, session);
  sqlite3_bind_text(s, 2, title, -1, SQLITE_TRANSIENT);
  return run(s);
}

static void copy_col(sqlite3_stmt *s, int col, char *out, size_t cap) {
  const char *v = (const char *)sqlite3_column_text(s, col);
  snprintf(out, cap, "%s", v ? v : "");
}

int store_list(Store *st, const char *filter, StoreItem *out, int max) {
  int n = 0;
  sqlite3_stmt *s = prepare(st, "SELECT s.id, substr(s.updated, 1, 16), s.title, "
                                "(SELECT count(*) FROM messages m WHERE m.session = s.id) FROM sessions s "
                                "WHERE ?1 = '' OR s.title LIKE '%' || ?1 || '%' OR EXISTS (SELECT 1 FROM messages m "
                                "WHERE m.session = s.id AND m.json LIKE '%' || ?1 || '%') "
                                "ORDER BY s.updated DESC, s.id DESC LIMIT ?2");
  if (!s) return 0;
  sqlite3_bind_text(s, 1, filter ? filter : "", -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(s, 2, max);
  while (n < max && sqlite3_step(s) == SQLITE_ROW) {
    out[n].id = (long)sqlite3_column_int64(s, 0);
    copy_col(s, 1, out[n].when, sizeof out[n].when);
    copy_col(s, 2, out[n].title, sizeof out[n].title);
    out[n].messages = sqlite3_column_int(s, 3);
    n++;
  }
  sqlite3_finalize(s);
  return n;
}

int store_load(Store *st, long session, History *h) {
  int n = 0;
  sqlite3_stmt *s = prepare(st, "SELECT 1 FROM sessions WHERE id = ?");
  if (!s) return -1;
  sqlite3_bind_int64(s, 1, session);
  if (sqlite3_step(s) != SQLITE_ROW) n = -1;
  sqlite3_finalize(s);
  if (n < 0 || !(s = prepare(st, "SELECT user, json FROM messages WHERE session = ? ORDER BY id"))) return -1;
  sqlite3_bind_int64(s, 1, session);
  while (sqlite3_step(s) == SQLITE_ROW) {
    const char *json = (const char *)sqlite3_column_text(s, 1);
    history_add(h, xstrdup(json ? json : "{}"), sqlite3_column_int(s, 0));
    n++;
  }
  sqlite3_finalize(s);
  return n;
}

void store_prune(Store *st, int keep) {
  sqlite3_stmt *s = prepare(st, "DELETE FROM sessions WHERE id NOT IN "
                                "(SELECT id FROM sessions ORDER BY updated DESC, id DESC LIMIT ?)");
  if (!s) return;
  sqlite3_bind_int(s, 1, keep);
  run(s);
}
