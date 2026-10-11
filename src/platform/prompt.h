/* System prompt for the agent, with the host machine described at startup (Win32 adapter). */
#ifndef TINYSHELL_PROMPT_H
#define TINYSHELL_PROMPT_H

/* Task-agnostic instructions plus detected OS, architecture, RAM, CPUs and current
 * directory. The prompt guides the model; it is not a safety control (see policy.h).
 * Caller frees. */
char *system_prompt(void);

#include "util.h"
void system_os(Buf *b);      /* "Windows 7 (6.1 build 7601, Service Pack 1)" */
void system_details(Buf *b); /* one line: OS, 32/64-bit, RAM, CPUs, current folder */

#endif
