/* The question tool: the model's question, its options in the menu and a last option to
 * type another answer. */
#ifndef TINYSHELL_QUESTION_H
#define TINYSHELL_QUESTION_H

#include "tools.h"

/* Asks a call that passed tool_problem(). Returns the tool result for the model: the
 * chosen option, the typed answer, or that the user closed the question. */
char *question_ask(const ToolCall *tc);

#endif
