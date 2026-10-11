/* One agent session: history, token counters and the round-trip loop with the local
 * approval gate (Win32 adapter). The model only proposes; every command is approved. */
#ifndef TINYSHELL_SESSION_H
#define TINYSHELL_SESSION_H

#include "conversation.h"
#include "provider.h"

typedef struct {
  History history;
  Provider provider;
  long tokens_in, tokens_out; /* summed from the API "usage" field */
  int turns, proposed, approved, rejected; /* counters for beta telemetry */
  unsigned long started_ms;
  char session_id[48];        /* random per conversation, sent in the provider's session header */
  char api_key[1024];
} Session;

void session_init(Session *s); /* system prompt + new session ID; provider and api_key are left as is */
void session_reset(Session *s); /* new conversation: history cleared, new session ID */
void session_free(Session *s); /* also wipes the API key */

/* Sends a user message and runs model round trips (tool call -> approval -> result)
 * until a text answer, an error or MAX_TURNS. Returns 0, or the HTTP status of a failed
 * request (401/403 mean the API key was rejected). */
unsigned long session_send(Session *s, const char *text);

#endif
