/* One agent session: history, token counters and the round-trip loop with the local
 * approval gate (Win32 adapter). The model only proposes; every command is approved. */
#ifndef TINYSHELL_SESSION_H
#define TINYSHELL_SESSION_H

#include "conversation.h"

typedef struct {
  History history;
  long tokens_in, tokens_out; /* summed from the API "usage" field */
  char session_id[48];        /* random per conversation, sent as x-opencode-session */
  char api_key[1024];
} Session;

void session_init(Session *s); /* system prompt + new session ID; api_key is left as is */
void session_reset(Session *s); /* new conversation: history cleared, new session ID */
void session_free(Session *s); /* also wipes the API key */

/* Sends a user message and runs model round trips (tool call -> approval -> result)
 * until a text answer, an error or MAX_TURNS. Returns 0, or the HTTP status of a failed
 * request (401/403 mean the API key was rejected). */
unsigned long session_send(Session *s, const char *text);

#endif
