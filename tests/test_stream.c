/* Regression tests for src/core/stream.c: SSE parsing with arbitrary chunk boundaries, joined
 * tool-call arguments, usage, keep-alives, CRLF, and servers that do not stream.
 * Run: ./build/test-core.sh */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "stream.h"
#include "check.h"

static char seen[512];
static void on_text(void *ctx, const char *t) {
  (void)ctx;
  strncat(seen, t, sizeof seen - strlen(seen) - 1);
}

static const char *SSE =
    ": keep-alive\r\n\r\n"
    "data: {\"choices\":[{\"delta\":{\"role\":\"assistant\",\"content\":\"Ho\"}}]}\r\n\r\n"
    "data: {\"choices\":[{\"delta\":{\"content\":\"la \\u00e1\"}}]}\n\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_9\",\"type\":\"function\","
    "\"function\":{\"name\":\"shell\",\"arguments\":\"{\\\"comm\"}}]}}]}\n\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"and\\\":\\\"ver\\\"}\"}}]}}]}\n\n"
    "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"tool_calls\"}]}\n\n"
    "data: {\"choices\":[],\"usage\":{\"prompt_tokens\":120,\"completion_tokens\":7}}\n\n"
    "data: [DONE]\n\n";

static void check_rebuilt(const char *resp) {
  Json j = {0}, a = {0};
  int msg, call;
  char *v, *args;
  CHECK(json_parse(&j, resp));
  msg = json_key(&j, json_index(&j, json_key(&j, 0, "choices"), 0), "message");
  v = json_str(&j, json_key(&j, msg, "content"));
  CHECK(!strcmp(v, "Hola \xc3\xa1"));
  free(v);
  call = json_index(&j, json_key(&j, msg, "tool_calls"), 0);
  v = json_str(&j, json_key(&j, call, "id"));
  CHECK(!strcmp(v, "call_9"));
  free(v);
  args = json_str(&j, json_key(&j, json_key(&j, call, "function"), "arguments"));
  CHECK(json_parse(&a, args)); /* the joined pieces form valid arguments */
  v = json_str(&a, json_key(&a, 0, "command"));
  CHECK(!strcmp(v, "ver"));
  free(v);
  free(args);
  v = json_str(&j, json_key(&j, json_index(&j, json_key(&j, 0, "choices"), 0), "finish_reason"));
  CHECK(!strcmp(v, "tool_calls"));
  free(v);
  v = json_str(&j, json_key(&j, json_key(&j, 0, "usage"), "prompt_tokens"));
  CHECK(!strcmp(v, "120"));
  free(v);
  json_free(&a);
  json_free(&j);
}

static void test_whole_and_bytewise(void) {
  Stream s;
  char *resp;
  size_t i, n = strlen(SSE);
  stream_init(&s);
  seen[0] = 0;
  stream_feed(&s, SSE, n, on_text, NULL);
  CHECK(s.done && !strcmp(seen, "Hola \xc3\xa1"));
  resp = stream_response(&s);
  check_rebuilt(resp);
  free(resp);
  stream_free(&s);

  stream_init(&s); /* one byte at a time: lines, JSON and UTF-8 split anywhere */
  seen[0] = 0;
  for (i = 0; i < n; i++) stream_feed(&s, SSE + i, 1, on_text, NULL);
  CHECK(!strcmp(seen, "Hola \xc3\xa1"));
  resp = stream_response(&s);
  check_rebuilt(resp);
  free(resp);
  stream_free(&s);
}

static void test_plain_json_server(void) {
  Stream s;
  char *resp;
  const char *body = "{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"sin streaming\"}}]}";
  stream_init(&s);
  stream_feed(&s, body, strlen(body), on_text, NULL);
  resp = stream_response(&s);
  CHECK(!strcmp(resp, body)); /* passed through untouched */
  free(resp);
  stream_free(&s);
}

static void test_text_only_and_missing_id(void) {
  Stream s;
  char *resp;
  Json j = {0};
  const char *sse = "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"function\":{\"name\":\"shell\",\"arguments\":\"{}\"}}]}}]}\n";
  stream_init(&s);
  stream_feed(&s, sse, strlen(sse), NULL, NULL);
  resp = stream_response(&s);
  CHECK(json_parse(&j, resp));
  CHECK(strstr(resp, "\"id\":\"call_0\"") != NULL); /* generated id */
  CHECK(strstr(resp, "\"content\":null") != NULL);
  json_free(&j);
  free(resp);
  stream_free(&s);
}

int main(void) {
  test_whole_and_bytewise();
  test_plain_json_server();
  test_text_only_and_missing_id();
  return CHECK_DONE("test_stream");
}
