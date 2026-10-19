/* Regression tests for src/core/tools.c and tool_problem() in src/core/policy.c: tool-call
 * extraction, unknown tools, malformed arguments, oversized commands and the question
 * tool's limits. Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "json.h"
#include "policy.h"
#include "tools.h"
#include "util.h"
#include "check.h"

static ToolCall parse_call(const char *response) {
  Json j = {0};
  ToolCall tc;
  int msg;
  CHECK(json_parse(&j, response));
  msg = json_key(&j, json_index(&j, json_key(&j, 0, "choices"), 0), "message");
  tool_call_parse(&j, json_index(&j, json_key(&j, msg, "tool_calls"), 0), &tc);
  json_free(&j);
  return tc;
}

static void test_tool_calls(void) {
  ToolCall tc;
  char longcmd[MAX_COMMAND + 32];
  tc = parse_call("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"c1\","
                  "\"type\":\"function\",\"function\":{\"name\":\"shell\",\"arguments\":\"{\\\"command\\\":\\\"ver\\\","
                  "\\\"reason\\\":\\\"versi\\\\u00f3n\\\"}\"}}]}}]}");
  CHECK(!strcmp(tc.id, "c1"));
  CHECK(!strcmp(tc.command, "ver"));
  CHECK(!strcmp(tc.reason, "versi\xc3\xb3n"));
  CHECK(tool_problem(&tc) == NULL);
  tool_call_free(&tc);

  /* arguments as an object instead of a string */
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"c2\",\"function\":{\"name\":\"shell\","
                  "\"arguments\":{\"command\":\"tasklist\",\"reason\":\"r\"}}}]}}]}");
  CHECK(!strcmp(tc.command, "tasklist"));
  CHECK(tool_problem(&tc) == NULL && !tc.admin);
  tool_call_free(&tc);

  /* admin: only the JSON literal true asks for elevation */
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"c5\",\"function\":{\"name\":\"shell\","
                  "\"arguments\":{\"command\":\"sc stop x\",\"reason\":\"r\",\"admin\":true}}}]}}]}");
  CHECK(tc.admin && tool_problem(&tc) == NULL);
  tool_call_free(&tc);
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"c6\",\"function\":{\"name\":\"shell\","
                  "\"arguments\":{\"command\":\"sc stop x\",\"reason\":\"r\",\"admin\":\"true\"}}}]}}]}");
  CHECK(!tc.admin);
  tool_call_free(&tc);

  /* unknown tool is refused even with a valid command */
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"c3\",\"function\":{\"name\":\"python\","
                  "\"arguments\":\"{\\\"command\\\":\\\"ver\\\"}\"}}]}}]}");
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);

  /* malformed arguments: nothing to run */
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"c4\",\"function\":{\"name\":\"shell\","
                  "\"arguments\":\"{\\\"command\\\": \\\"del\"}}]}}]}");
  CHECK(!strcmp(tc.command, ""));
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);

  /* missing function and id */
  tc = parse_call("{\"choices\":[{\"message\":{\"tool_calls\":[{}]}}]}");
  CHECK(!strcmp(tc.id, "") && !strcmp(tc.name, "") && !strcmp(tc.command, ""));
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);

  /* control characters and oversized commands are refused before approval */
  memset(&tc, 0, sizeof tc);
  tc.id = xstrdup("x");
  tc.name = xstrdup("shell");
  tc.reason = xstrdup("");
  tc.command = xstrdup("ver\r\ndel /q c:\\x");
  CHECK(tool_problem(&tc) != NULL);
  free(tc.command);
  memset(longcmd, 'a', sizeof longcmd - 1);
  longcmd[sizeof longcmd - 1] = 0;
  tc.command = xstrdup(longcmd);
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
}

static ToolCall question_call(const char *args) {
  Buf b = {0};
  char *e = jesc(args);
  ToolCall tc;
  buf_str(&b, "{\"choices\":[{\"message\":{\"tool_calls\":[{\"id\":\"q1\",\"function\":{\"name\":\"question\",\"arguments\":\"");
  buf_str(&b, e);
  buf_str(&b, "\"}}]}}]}");
  tc = parse_call(b.p);
  free(e);
  free(b.p);
  return tc;
}

static void test_question(void) {
  ToolCall tc = question_call("{\"question\":\"\\u00bfQu\\u00e9 disco?\\nElija uno\",\"options\":[\"C:\",\"D:\"]}");
  char big[MAX_OPTION + 2];
  Buf many = {0};
  int i;
  CHECK(!strcmp(tc.question, "\xc2\xbfQu\xc3\xa9 disco?\nElija uno") && tc.noptions == 2);
  CHECK(!strcmp(tc.options[0], "C:") && !strcmp(tc.options[1], "D:"));
  CHECK(!strcmp(tc.command, "") && tool_problem(&tc) == NULL); /* a line break is fine in the question */
  tool_call_free(&tc);

  tc = question_call("{\"question\":\"x\",\"options\":[\"solo\"]}"); /* fewer than 2 */
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  tc = question_call("{\"question\":\"\",\"options\":[\"a\",\"b\"]}");
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  tc = question_call("{\"question\":\"x\",\"options\":\"a, b\"}"); /* not an array */
  CHECK(tc.noptions == 0 && tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  tc = question_call("{\"question\":\"x\",\"options\":[\"a\",\"b\\nc\"]}"); /* an option is one row */
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  tc = question_call("{\"question\":\"x\",\"options\":[\"a\",\"\"]}");
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  tc = question_call("{\"question\":\"x\",\"options\":[\"a\",7]}"); /* a number reads as "7" */
  CHECK(tool_problem(&tc) == NULL);
  tool_call_free(&tc);

  memset(big, 'o', sizeof big - 1);
  big[sizeof big - 1] = 0;
  buf_str(&many, "{\"question\":\"x\",\"options\":[\"a\",\"");
  buf_str(&many, big);
  buf_str(&many, "\"]}");
  tc = question_call(many.p);
  CHECK(tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  free(buf_take(&many));

  buf_str(&many, "{\"question\":\"x\",\"options\":[\"o\"");
  for (i = 0; i < MAX_OPTIONS + 3; i++) buf_str(&many, ",\"o\"");
  buf_str(&many, "]}");
  tc = question_call(many.p); /* too many: counted, only MAX_OPTIONS kept, all freed */
  CHECK(tc.noptions == MAX_OPTIONS + 4 && tool_problem(&tc) != NULL);
  tool_call_free(&tc);
  free(many.p);
}

int main(void) {
  test_tool_calls();
  test_question();
  return CHECK_DONE("test_tools");
}
