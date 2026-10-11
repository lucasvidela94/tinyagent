/* System prompt for the agent, with the host machine described at startup (Win32 adapter). */
#ifndef TINYSHELL_PROMPT_H
#define TINYSHELL_PROMPT_H

/* Task-agnostic instructions plus detected OS, architecture, RAM, CPUs and current
 * directory. The prompt guides the model; it is not a safety control (see policy.h).
 * Caller frees. */
char *system_prompt(void);

#endif
