#include "tools.h"

#include <stdlib.h>

void tools_json(Buf *b, const Dialect *d) {
  buf_str(b, "\"tools\":[{\"type\":\"function\",\"function\":{\"name\":\"shell\",\"description\":\"Run one ");
  buf_str(b, d->shell);
  buf_str(b, " command on the local ");
  buf_str(b, d->os);
  buf_str(b, " machine after the user approves it. Use it to inspect the system and files and to make changes the "
             "user agreed to. No interactive input; time-limited; long output is cut.\","
             "\"parameters\":{\"type\":\"object\",\"properties\":{"
             "\"command\":{\"type\":\"string\",\"description\":\"Exact command to run in ");
  buf_str(b, d->shell);
  buf_str(b, "\"},\"reason\":{\"type\":\"string\",\"description\":\"Why this command is needed\"}},"
             "\"required\":[\"command\",\"reason\"],\"additionalProperties\":false}}}],\"tool_choice\":\"auto\"");
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
