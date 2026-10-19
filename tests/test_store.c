/* Regression tests for src/core/store.c (saved conversations) on an in-memory SQLite
 * database: sessions, titles, listing, search, loading into a history and pruning.
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "conversation.h"
#include "store.h"
#include "check.h"

static void add_user(Store *st, long id, const char *text) {
  char *json = message_json("user", text);
  CHECK(store_add(st, id, json, 1, text));
  free(json);
}

static void test_sessions(void) {
  Store *st = store_open(":memory:");
  StoreItem items[8];
  History h = {0};
  long a, b;
  char *json;
  CHECK(st != NULL);
  a = store_new_session(st, "deepseek-v4.1-flash");
  b = store_new_session(st, "kimi-k2.7-code");
  CHECK(a > 0 && b > a);
  add_user(st, a, "\xc2\xbfqu\xc3\xa9 programas arrancan con Windows?\nlistalos");
  json = message_json("assistant", "Tengo 23 programas de inicio.");
  CHECK(store_add(st, a, json, 0, NULL));
  free(json);
  add_user(st, b, "free space on C:");
  add_user(st, b, "second message does not change the title");

  CHECK(store_list(st, "", items, 8) == 2);
  CHECK(items[0].id == b && !strcmp(items[0].title, "free space on C:") && items[0].messages == 2);
  CHECK(items[1].id == a && items[1].messages == 2);
  CHECK(strchr(items[1].title, '\n') == NULL); /* one line */
  CHECK(strlen(items[0].when) == 16);

  CHECK(store_list(st, "PROGRAMAS DE INICIO", items, 8) == 1 && items[0].id == a); /* in a message, any case */
  CHECK(store_list(st, "free space", items, 8) == 1 && items[0].id == b);
  CHECK(store_list(st, "nothing like this", items, 8) == 0);
  CHECK(store_list(st, "", items, 1) == 1); /* max respected */

  history_add(&h, message_json("system", "sys"), 0);
  CHECK(store_load(st, a, &h) == 2);
  CHECK(h.n == 3 && h.m[1].user && !h.m[2].user);
  CHECK(strstr(h.m[2].json, "23 programas") != NULL);
  CHECK(store_load(st, 999, &h) == -1 && h.n == 3);
  history_free(&h);

  store_prune(st, 1); /* keeps the most recently used: b */
  CHECK(store_list(st, "", items, 8) == 1 && items[0].id == b);
  memset(&h, 0, sizeof h);
  CHECK(store_load(st, a, &h) == -1); /* its messages went with it */
  history_free(&h);
  store_close(st);
}

static void test_long_title(void) {
  Store *st = store_open(":memory:");
  StoreItem it;
  char text[400];
  long id = store_new_session(st, "m");
  memset(text, 'x', sizeof text - 1);
  text[sizeof text - 1] = 0;
  add_user(st, id, text);
  CHECK(store_list(st, "", &it, 1) == 1 && strlen(it.title) < sizeof it.title && strlen(it.title) > 20);
  store_close(st);
  CHECK(store_open("/nonexistent-dir/x/y.db") == NULL); /* a path that cannot be created */
}

int main(void) {
  test_sessions();
  test_long_title();
  return CHECK_DONE("test_store");
}
