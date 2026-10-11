#include "stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "json.h"

void stream_init(Stream *s) { memset(s, 0, sizeof *s); }

void stream_free(Stream *s) {
  int i;
  free(s->line.p);
  free(s->raw.p);
  free(s->content.p);
  for (i = 0; i < STREAM_MAX_CALLS; i++) {
    free(s->calls[i].id.p);
    free(s->calls[i].name.p);
    free(s->calls[i].args.p);
  }
  memset(s, 0, sizeof *s);
}

static void take_str(Buf *dst, const Json *j, int tok, int replace) {
  char *v;
  if (json_type(j, tok) != '"') return;
  v = json_str(j, tok);
  if (replace) dst->len = 0; /* ids and names arrive whole (some servers repeat them) */
  if (*v) buf_str(dst, v);
  free(v);
}

static void tool_deltas(Stream *s, const Json *j, int calls) {
  int i;
  for (i = 0; i < json_size(j, calls); i++) {
    int c = json_index(j, calls, i), fn = json_key(j, c, "function");
    char *idx = json_str(j, json_key(j, c, "index"));
    int k = *idx ? atoi(idx) : i;
    free(idx);
    if (k < 0 || k >= STREAM_MAX_CALLS) continue;
    if (k >= s->ncalls) s->ncalls = k + 1;
    take_str(&s->calls[k].id, j, json_key(j, c, "id"), 1);
    take_str(&s->calls[k].name, j, json_key(j, fn, "name"), 1);
    take_str(&s->calls[k].args, j, json_key(j, fn, "arguments"), 0);
  }
}

static void event(Stream *s, const char *payload, StreamText on_text, void *ctx) {
  Json j = {0};
  int choice, delta, usage;
  if (!strcmp(payload, "[DONE]")) {
    s->done = 1;
    return;
  }
  if (!json_parse(&j, payload)) return;
  s->events++;
  choice = json_index(&j, json_key(&j, 0, "choices"), 0);
  delta = json_key(&j, choice, "delta");
  if (json_type(&j, json_key(&j, delta, "content")) == '"') {
    char *text = json_str(&j, json_key(&j, delta, "content"));
    if (*text) {
      buf_str(&s->content, text);
      if (on_text) on_text(ctx, text);
    }
    free(text);
  }
  tool_deltas(s, &j, json_key(&j, delta, "tool_calls"));
  if (json_type(&j, json_key(&j, choice, "finish_reason")) == '"') {
    char *f = json_str(&j, json_key(&j, choice, "finish_reason"));
    snprintf(s->finish, sizeof s->finish, "%s", f);
    free(f);
  }
  if ((usage = json_key(&j, 0, "usage")) >= 0 && json_type(&j, usage) == '{') {
    char *in = json_str(&j, json_key(&j, usage, "prompt_tokens"));
    char *out = json_str(&j, json_key(&j, usage, "completion_tokens"));
    s->prompt_tokens = atol(in);
    s->completion_tokens = atol(out);
    free(in);
    free(out);
  }
  json_free(&j);
}

static void line(Stream *s, char *l, StreamText on_text, void *ctx) {
  size_t n = strlen(l);
  if (n && l[n - 1] == '\r') l[--n] = 0;
  if (strncmp(l, "data:", 5)) return; /* comments (": keep-alive"), "event:", blank lines */
  l += 5;
  if (*l == ' ') l++;
  event(s, l, on_text, ctx);
}

void stream_feed(Stream *s, const char *data, size_t n, StreamText on_text, void *ctx) {
  size_t i, start = 0;
  size_t dropped = 0;
  buf_add_capped(&s->raw, data, n, MAX_HTTP, &dropped); /* for a server that did not stream */
  for (i = 0; i < n; i++) {
    if (data[i] != '\n') continue;
    buf_add(&s->line, data + start, i - start);
    line(s, s->line.p ? s->line.p : (char *)"", on_text, ctx);
    s->line.len = 0;
    start = i + 1;
  }
  if (start < n && s->line.len + (n - start) <= MAX_HTTP) buf_add(&s->line, data + start, n - start);
}

static void str_field(Buf *b, const char *key, const char *value) {
  char *e = jesc(value);
  buf_str(b, "\"");
  buf_str(b, key);
  buf_str(b, "\":\"");
  buf_str(b, e);
  buf_str(b, "\"");
  free(e);
}

char *stream_response(const Stream *s) {
  Buf b = {0};
  char num[96];
  int i;
  if (!s->events) return xstrdup(s->raw.p ? s->raw.p : "");
  buf_str(&b, "{\"choices\":[{\"message\":{\"role\":\"assistant\",");
  if (s->content.len) str_field(&b, "content", s->content.p);
  else buf_str(&b, "\"content\":null");
  if (s->ncalls) {
    buf_str(&b, ",\"tool_calls\":[");
    for (i = 0; i < s->ncalls; i++) {
      char id[32];
      snprintf(id, sizeof id, "call_%d", i); /* if the server sent none */
      buf_str(&b, i ? ",{" : "{");
      str_field(&b, "id", s->calls[i].id.len ? s->calls[i].id.p : id);
      buf_str(&b, ",\"type\":\"function\",\"function\":{");
      str_field(&b, "name", s->calls[i].name.p ? s->calls[i].name.p : "");
      buf_str(&b, ",");
      str_field(&b, "arguments", s->calls[i].args.p ? s->calls[i].args.p : "");
      buf_str(&b, "}}");
    }
    buf_str(&b, "]");
  }
  buf_str(&b, "},");
  str_field(&b, "finish_reason", s->finish);
  snprintf(num, sizeof num, "}],\"usage\":{\"prompt_tokens\":%ld,\"completion_tokens\":%ld}}", s->prompt_tokens,
           s->completion_tokens);
  buf_str(&b, num);
  return buf_take(&b);
}
