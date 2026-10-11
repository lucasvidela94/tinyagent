/* Self-update from the latest GitHub release. Portable: the platform gives the binary's
 * path, renames, the release asset name (sys.h) and the signature check (verify.h).
 * Never automatic: only `tinyshell --update` or /actualizar start it, and it asks before
 * installing. Does not need the API key or the model, so it works even when the agent loop
 * is broken. */
#ifndef TINYSHELL_UPDATE_H
#define TINYSHELL_UPDATE_H

/* Checks version.txt; if newer and the user agrees, downloads the platform's binary + .sig,
 * verifies the RSA signature, self-tests the new binary (`--version` must run on this
 * system and report the announced, newer version) and swaps it in, keeping the previous
 * one next to it (tinyshell.old.exe on Windows). Returns 0 if up to date or updated, 1 otherwise. */
int update_run(void);

/* Swaps the binary and its .old sibling (so a second rollback rolls forward). */
int update_rollback(void);

#endif
