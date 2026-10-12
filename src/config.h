/* Version, release URL and resource bounds (the model provider is in provider.c / config.ini). Tuned for a 2 GB Atom N2600 on Windows 7 x86. */
#ifndef TINYSHELL_CONFIG_H
#define TINYSHELL_CONFIG_H

#define TINYSHELL_VERSION "0.8.0"
#define TINYSHELL_WIDEN_(s) L##s
#define TINYSHELL_WIDEN(s) TINYSHELL_WIDEN_(s)
#define TINYSHELL_VERSION_W TINYSHELL_WIDEN(TINYSHELL_VERSION)
/* Release assets: version.txt, tinyshell.exe, tinyshell.exe.sig, install.bat, SHA256SUMS.
 * Must stay https://: downloads are authenticated by signature, but version.txt is not. */
/* Problem reports and beta telemetry (relay/ in this repo, a Cloudflare Worker). */
#define RELAY_URL "https://tinyshell-relay.lucasan-videla.workers.dev"
#define REPORT_EMAIL "lucasan.videla@gmail.com" /* fallback when the relay cannot be reached */
#define UPDATE_BASE_URL "https://github.com/lucasvidela94/tinyagent/releases/latest/download/"

#define MAX_HTTP (1024 * 1024)   /* API response body, bytes */
#define MAX_UPDATE (4 * 1024 * 1024) /* downloaded release binary, bytes */
#define MAX_HISTORY (192 * 1024) /* soft budget for the messages array, bytes */
#define MAX_OUTPUT 24000         /* command output kept, bytes */
#define MAX_COMMAND 2000         /* proposed command, bytes */
#define MAX_LINE 4096            /* typed line, bytes */
#define MAX_TURNS 25             /* model round trips per user message (each tool call is approved) */
#define MAX_TOKENS 20000         /* JSON tokens per document */
#define MAX_DEPTH 64             /* JSON nesting */
#ifndef CMD_TIMEOUT_MS
#define CMD_TIMEOUT_MS 180000    /* command wall-clock limit (tests may lower it) */
#endif

#endif
