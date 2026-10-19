/* The approval gate for shell commands, like OpenCode's: the command in a panel, then a
 * menu with yes once, yes always for its prefix in this session, or no with what the model
 * should do instead. Deny rules in config.ini refuse before asking. */
#ifndef TINYSHELL_APPROVAL_H
#define TINYSHELL_APPROVAL_H

#include "session.h"
#include "tools.h"

/* Asks about a call that passed tool_problem(), runs it if approved and returns the tool
 * result for the model: the output, or why nothing ran. */
char *approval_run(Session *s, const ToolCall *tc);

#endif
