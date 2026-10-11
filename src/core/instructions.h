/* The task-agnostic instructions at the top of the system prompt, with the shell-specific
 * parts taken from a dialect. Portable C. The prompt guides the model; it is not a safety
 * control (see policy.h). Written in English, which models follow most precisely; see
 * docs/SYSTEM_PROMPT.md. */
#ifndef TINYSHELL_INSTRUCTIONS_H
#define TINYSHELL_INSTRUCTIONS_H

#include "dialect.h"

char *instructions(const Dialect *d); /* caller frees */

#endif
