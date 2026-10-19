#include "tools.h"

#include <stdio.h>
#include <stdlib.h>

static void shell_json(Buf *b, const Dialect *d) {
  buf_str(b, "{\"type\":\"function\",\"function\":{\"name\":\"shell\",\"description\":\"Run one ");
  buf_str(b, d->shell);
  buf_str(b, " command on the local ");
  buf_str(b, d->os);
  buf_str(b, " machine after the user approves it. Use it to inspect the system and files and to make changes the "
             "user agreed to. No interactive input; time-limited; long output is cut.\","
             "\"parameters\":{\"type\":\"object\",\"properties\":{"
             "\"command\":{\"type\":\"string\",\"description\":\"Exact command to run in ");
  buf_str(b, d->shell);
  buf_str(b, "\"},\"reason\":{\"type\":\"string\",\"description\":\"Why this command is needed\"}},"
             "\"required\":[\"command\",\"reason\"],\"additionalProperties\":false}}}");
}

static void question_json(Buf *b) {
  char n[16];
  buf_str(b, "{\"type\":\"function\",\"function\":{\"name\":\"question\",\"description\":\"Ask the user to pick one of "
             "a few options when the choice is theirs to make, instead of guessing. The user can also type another "
             "answer. Not for asking permission to run a command: every command is approved anyway.\","
             "\"parameters\":{\"type\":\"object\",\"properties\":{"
             "\"question\":{\"type\":\"string\",\"description\":\"The question, short and plain\"},"
             "\"options\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},\"minItems\":2,\"maxItems\":");
  snprintf(n, sizeof n, "%d", MAX_OPTIONS);
  buf_str(b, n);
  buf_str(b, ",\"description\":\"The choices, a few words each\"}},"
             "\"required\":[\"question\",\"options\"],\"additionalProperties\":false}}}");
}

void tools_json(Buf *b, const Dialect *d) {
  buf_str(b, "\"tools\":[");
  shell_json(b, d);
  buf_str(b, ",");
  question_json(b);
  buf_str(b, "],\"tool_choice\":\"auto\"");
}

void tool_call_parse(const Json *j, int call, ToolCall *tc) {
  int fn = json_key(j, call, "function"), opts, i;
  char *args = json_str(j, json_key(j, fn, "arguments")); /* a JSON string or an object */
  Json a = {0};
  json_parse(&a, args); /* if it fails, a is empty and every argument becomes "" */
  tc->id = json_str(j, json_key(j, call, "id"));
  tc->name = json_str(j, json_key(j, fn, "name"));
  tc->command = json_str(&a, json_key(&a, 0, "command"));
  tc->reason = json_str(&a, json_key(&a, 0, "reason"));
  tc->question = json_str(&a, json_key(&a, 0, "question"));
  opts = json_key(&a, 0, "options");
  tc->noptions = json_type(&a, opts) == '[' ? json_size(&a, opts) : 0;
  for (i = 0; i < tc->noptions && i < MAX_OPTIONS; i++) tc->options[i] = json_str(&a, json_index(&a, opts, i));
  json_free(&a);
  free(args);
}

void tool_call_free(ToolCall *tc) {
  int i;
  free(tc->id);
  free(tc->name);
  free(tc->command);
  free(tc->reason);
  free(tc->question);
  for (i = 0; i < tc->noptions && i < MAX_OPTIONS; i++) free(tc->options[i]);
}
