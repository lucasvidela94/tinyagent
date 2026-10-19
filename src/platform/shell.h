/* Runs one approved command through cmd.exe and captures its output (Win32 adapter). */
#ifndef TINYSHELL_SHELL_H
#define TINYSHELL_SHELL_H

/* Runs `cmd.exe /d /s /c "<cmd>"` from the system directory (no AutoRun, no extra shell),
 * with stdin = NUL, stdout+stderr merged and a CMD_TIMEOUT_MS limit. Returns UTF-8 text
 * (first MAX_OUTPUT bytes) followed by "[exit=N]", truncation or timeout notes. Never NULL.
 * Must only be called after local approval. */
char *run_command(const char *cmd);

/* Like run_command, but with administrator rights: on Windows through the UAC prompt
 * ("runas"), the output read back from a temporary file. Says so, and runs nothing, when
 * the user declines or the platform has no such mode. Must only be called after local
 * approval of this command. */
char *run_command_admin(const char *cmd);

#endif
