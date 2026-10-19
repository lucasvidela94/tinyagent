/* Saved conversations for a session, through the store (src/core/store.c, ADR 0002):
 * every message is saved as it is added, and /sessions lists, searches and resumes them.
 * Saving is on unless config.ini says [sessions] save = 0. */
#ifndef TINYSHELL_SAVED_H
#define TINYSHELL_SAVED_H

#include "session.h"

#define SAVED_KEEP 50 /* sessions kept; older ones are deleted when TinyShell starts */

void saved_open(Session *s);  /* after session_init; prunes, and mentions /sessions if any exist */
void saved_close(Session *s);

/* Adds a message to the history (taking ownership of json) and saves it. `text` is the
 * user's words for a user message (it titles a new session), NULL otherwise. The
 * conversation is created in the store with its first message. */
void saved_add(Session *s, char *json, int user, const char *text);

/* "/sessions [text]": the saved conversations (those containing `filter`, if any) in a
 * menu; the one picked replaces the current conversation. */
void saved_pick(Session *s, const char *filter);

#endif
