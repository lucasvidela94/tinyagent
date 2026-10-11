#include "conversation.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

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

void history_trim(History *h, size_t budget) {
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

static const char *TOOLS =
    "\"tools\":[{\"type\":\"function\",\"function\":{\"name\":\"shell\","
    "\"description\":\"Run one cmd.exe command on the local Windows machine after the user approves it. "
    "Use it to inspect the system and files and to make changes the user agreed to. No interactive input; "
    "time-limited; long output is cut.\","
    "\"parameters\":{\"type\":\"object\",\"properties\":{"
    "\"command\":{\"type\":\"string\",\"description\":\"Exact command to run in cmd.exe\"},"
    "\"reason\":{\"type\":\"string\",\"description\":\"Why this command is needed\"}},"
    "\"required\":[\"command\",\"reason\"],\"additionalProperties\":false}}}],\"tool_choice\":\"auto\"";

char *build_request(const History *h, const char *model) {
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
  buf_str(&b, "],\"temperature\":0.1,\"max_tokens\":4096,");
  buf_str(&b, TOOLS);
  buf_str(&b, "}");
  return buf_take(&b);
}

void tool_call_parse(const Json *j, int call, ToolCall *tc) {
  int fn = json_key(j, call, "function");
  char *args = json_str(j, json_key(j, fn, "arguments")); /* a JSON string or an object */
  Json a = {0};
  tc->id = json_str(j, json_key(j, call, "id"));
  tc->name = json_str(j, json_key(j, fn, "name"));
  if (json_parse(&a, args)) {
    tc->command = json_str(&a, json_key(&a, 0, "command"));
    tc->reason = json_str(&a, json_key(&a, 0, "reason"));
  } else {
    tc->command = xstrdup("");
    tc->reason = xstrdup("");
  }
  json_free(&a);
  free(args);
}

void tool_call_free(ToolCall *tc) {
  free(tc->id);
  free(tc->name);
  free(tc->command);
  free(tc->reason);
}
