/* Self-update from the latest GitHub release (Win32 adapter). Never automatic: only
 * `tinyshell --update` or /actualizar start it, and it asks before installing. Does not
 * need the API key or the model, so it works even when the agent loop is broken. */
#ifndef TINYSHELL_UPDATE_H
#define TINYSHELL_UPDATE_H

/* Checks version.txt; if newer and the user agrees, downloads tinyshell.exe + .sig,
 * verifies the RSA signature, self-tests the new binary (`--version` must run on this
 * Windows and report the announced, newer version) and swaps it in, keeping the previous
 * one as tinyshell.old.exe. Returns 0 if up to date or updated, 1 otherwise. */
int update_run(void);

/* Swaps tinyshell.exe and tinyshell.old.exe (so a second rollback rolls forward). */
int update_rollback(void);

#endif
