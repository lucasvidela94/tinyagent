/* The chat's slash commands. Canonical names are English (/help, /model, /login, /report,
 * /reset, /update, /quit); the earlier Spanish names stay as aliases. */
#ifndef TINYSHELL_SLASH_H
#define TINYSHELL_SLASH_H

#include "session.h"

enum { SLASH_NONE, SLASH_DONE, SLASH_QUIT };

/* SLASH_NONE if input is not a slash command (send it to the model). */
int slash_run(Session *s, const char *input);

#endif
