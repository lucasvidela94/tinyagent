/* Chat Completions protocol: message history, request body and tool-call extraction. Portable C. */
#ifndef TINYSHELL_CONVERSATION_H
#define TINYSHELL_CONVERSATION_H

#include <stddef.h>

#include "json.h"

/* One JSON object per message; m[0] is the system prompt. Zero-initialize before use. */
typedef struct { char *json; int user; } Msg;
typedef struct { Msg *m; int n, cap; size_t bytes; } History;

void history_add(History *h, char *json, int user); /* takes ownership of json */
/* Drops the oldest whole exchanges (a user message up to the next one), so an assistant
 * tool_calls message is never separated from its tool results. The system prompt and
 * the latest exchange always stay, so the budget is soft. */
void history_trim(History *h, size_t budget);
void history_reset(History *h); /* keeps only the system prompt */
void history_free(History *h);

char *message_json(const char *role, const char *content);
char *tool_result_json(const char *id, const char *output);
char *build_request(const History *h); /* full POST body with the shell tool declared */

/* One entry of message.tool_calls. Missing pieces become "", never NULL. */
typedef struct { char *id, *name, *command, *reason; } ToolCall;
void tool_call_parse(const Json *j, int call, ToolCall *tc);
void tool_call_free(ToolCall *tc);

#endif
