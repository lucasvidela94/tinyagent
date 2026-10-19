#include "conversation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "i18n.h"
#include "json.h"
#include "tools.h"
#include "util.h"

#define KEEP_RESULTS 2 /* latest tool results always sent verbatim */
#define ELIDE_MIN 1024 /* smaller results are not worth replacing */

void history_add(History *h, char *json, int user) {
  if (h->n == h->cap) {
    h->cap = h->cap ? h->cap * 2 : 16;
    h->m = (Msg *)xrealloc(h->m, (size_t)h->cap * sizeof(Msg));
  }
  h->m[h->n].json = json;
  h->m[h->n].user = user;
  h->n++;
  h->bytes += strlen(json) + 1;
}

static void drop(History *h, int from, int to) { /* removes m[from..to) */
  int i;
  for (i = from; i < to; i++) {
    h->bytes -= strlen(h->m[i].json) + 1;
    free(h->m[i].json);
  }
  memmove(&h->m[from], &h->m[to], (size_t)(h->n - to) * sizeof(Msg));
  h->n -= to - from;
}

static int is_tool_result(const Msg *m) { return !strncmp(m->json, "{\"role\":\"tool\"", 14); }

/* Keeps the last line (the exit status), so the model still knows how the command ended. */
static void elide(History *h, int i) {
  Json j = {0};
  Buf b = {0};
  char *id, *out, *tail, head[160];
  if (!json_parse(&j, h->m[i].json)) return;
  id = json_str(&j, json_key(&j, 0, "tool_call_id"));
  out = json_str(&j, json_key(&j, 0, "content"));
  json_free(&j);
  snprintf(head, sizeof head, tr("[output of %lu bytes left out to save context; run the command again if you need it]"),
           (unsigned long)strlen(out));
  buf_str(&b, head);
  tail = strrchr(out, '\n');
  if (tail && strlen(tail) < 200) buf_str(&b, tail);
  h->bytes -= strlen(h->m[i].json) + 1;
  free(h->m[i].json);
  h->m[i].json = tool_result_json(id, b.p ? b.p : "");
  h->bytes += strlen(h->m[i].json) + 1;
  free(b.p);
  free(id);
  free(out);
}

/* Old command outputs are the bulk of a long session and the model rarely needs them again,
 * so they go first, oldest first, before any whole exchange is dropped. */
static void elide_old_results(History *h, size_t budget) {
  int i, last, kept = 0;
  for (last = h->n - 1; last > 0 && kept < KEEP_RESULTS; last--)
    if (is_tool_result(&h->m[last])) kept++;
  for (i = 1; i <= last && h->bytes > budget; i++)
    if (is_tool_result(&h->m[i]) && strlen(h->m[i].json) > ELIDE_MIN) elide(h, i);
}

void history_trim(History *h, size_t budget) {
  elide_old_results(h, budget);
  while (h->bytes > budget) {
    int i, next = -1;
    for (i = 2; i < h->n; i++)
      if (h->m[i].user) { next = i; break; }
    if (next < 0) break;
    drop(h, 1, next);
  }
}

void history_reset(History *h) {
  if (h->n > 1) drop(h, 1, h->n);
}

void history_free(History *h) {
  drop(h, 0, h->n);
  free(h->m);
  h->m = NULL;
  h->cap = 0;
}

char *message_json(const char *role, const char *content) {
  Buf b = {0};
  char *e = jesc(content);
  buf_str(&b, "{\"role\":\"");
  buf_str(&b, role);
  buf_str(&b, "\",\"content\":\"");
  buf_str(&b, e);
  buf_str(&b, "\"}");
  free(e);
  return buf_take(&b);
}

char *tool_result_json(const char *id, const char *output) {
  Buf b = {0};
  char *ei = jesc(id), *eo = jesc(output);
  buf_str(&b, "{\"role\":\"tool\",\"tool_call_id\":\"");
  buf_str(&b, ei);
  buf_str(&b, "\",\"content\":\"");
  buf_str(&b, eo);
  buf_str(&b, "\"}");
  free(ei);
  free(eo);
  return buf_take(&b);
}

char *build_request(const History *h, const char *model, const Dialect *d) {
  Buf b = {0};
  char *em = jesc(model);
  int i;
  buf_str(&b, "{\"model\":\"");
  buf_str(&b, em);
  buf_str(&b, "\",\"messages\":[");
  free(em);
  for (i = 0; i < h->n; i++) {
    if (i) buf_str(&b, ",");
    buf_str(&b, h->m[i].json);
  }
  buf_str(&b, "],\"stream\":true,\"stream_options\":{\"include_usage\":true},\"max_tokens\":4096,");
  tools_json(&b, d);
  buf_str(&b, "}");
  return buf_take(&b);
}
