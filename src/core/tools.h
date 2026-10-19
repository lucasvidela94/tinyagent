/* The tools the model may call: their schemas in the request and their calls in the
 * response. Portable C. */
#ifndef TINYSHELL_TOOLS_H
#define TINYSHELL_TOOLS_H

#include "dialect.h"
#include "json.h"
#include "util.h"

#define MAX_OPTIONS 8     /* question choices: with "other answer" they fit the 10-row menu */
#define MAX_OPTION 120    /* bytes per choice, one menu row */
#define MAX_QUESTION 1000 /* bytes of question text */

/* Appends "tools":[...],"tool_choice":"auto": shell, described for the platform's shell
 * (dialect.h), and question. */
void tools_json(Buf *b, const Dialect *d);

/* One entry of message.tool_calls, every argument read whatever the tool: missing
 * strings become "", never NULL. noptions is the real count of options, but at most
 * MAX_OPTIONS of them are kept, so more than MAX_OPTIONS means too many. */
typedef struct {
  char *id, *name;
  char *command, *reason;                         /* shell */
  char *question, *options[MAX_OPTIONS]; int noptions; /* question */
} ToolCall;
void tool_call_parse(const Json *j, int call, ToolCall *tc);
void tool_call_free(ToolCall *tc);

#endif
