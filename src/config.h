/* Endpoint, model and resource bounds. Tuned for a 2 GB Atom N2600 on Windows 7 x86. */
#ifndef TINYSHELL_CONFIG_H
#define TINYSHELL_CONFIG_H

#define API_HOST L"opencode.ai"
#define API_PATH L"/zen/go/v1/chat/completions"
#define MODEL "deepseek-v4.1-flash"

#define MAX_HTTP (1024 * 1024)   /* API response body, bytes */
#define MAX_HISTORY (192 * 1024) /* soft budget for the messages array, bytes */
#define MAX_OUTPUT 24000         /* command output kept, bytes */
#define MAX_COMMAND 2000         /* proposed command, bytes */
#define MAX_LINE 4096            /* typed line, bytes */
#define MAX_TURNS 8              /* model round trips per user message */
#define MAX_TOKENS 20000         /* JSON tokens per document */
#define MAX_DEPTH 64             /* JSON nesting */
#define CMD_TIMEOUT_MS 180000    /* command wall-clock limit */

#endif
