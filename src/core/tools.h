/* The tools the model may call: their schemas in the request and their calls in the
 * response. Portable C. */
#ifndef TINYSHELL_TOOLS_H
#define TINYSHELL_TOOLS_H

#include "dialect.h"
#include "json.h"
#include "util.h"

/* Appends "tools":[...],"tool_choice":"auto", the shell described for the platform's
 * shell (dialect.h). */
void tools_json(Buf *b, const Dialect *d);

/* One entry of message.tool_calls. Missing pieces become "", never NULL. */
typedef struct { char *id, *name, *command, *reason; } ToolCall;
void tool_call_parse(const Json *j, int call, ToolCall *tc);
void tool_call_free(ToolCall *tc);

#endif
