/* Local approval policy for proposed commands. Portable C.
 * The model is an untrusted recommender: these checks run in local code, never in the prompt. */
#ifndef TINYSHELL_POLICY_H
#define TINYSHELL_POLICY_H

#include "conversation.h"
#include "dialect.h"

/* Refuses a proposal before the user is asked (unknown tool, empty, oversized, control
 * characters). Returns the message (English; show it through tr()), or NULL if it may be
 * shown for approval. */
const char *command_problem(const ToolCall *tc);

/* Keyword heuristic, per shell dialect, that demands a second, typed confirmation. It is
 * NOT a security boundary: the per-command approval is. Returns a short Spanish reason, or NULL. */
const char *risk_reason(const Dialect *d, const char *cmd);

/* Only "s", "si", "sí", "y" or "yes" (any case, surrounding blanks allowed) count as approval. */
int is_yes(const char *answer);

#endif
