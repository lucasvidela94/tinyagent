/* Opt-in API key storage, removable with `tinyshell logout`.
 * Windows: Credential Manager, encrypted with the user's logon (DPAPI), as
 * "TinyShell/<provider>"; also visible in Control Panel > Credential Manager.
 * Linux: ~/.config/tinyshell/credentials, mode 0600 (old distros have no common keyring). */
#ifndef TINYSHELL_CREDENTIAL_H
#define TINYSHELL_CREDENTIAL_H

#include <stddef.h>

extern const char CREDENTIAL_WHERE[]; /* for messages: "el Administrador de credenciales de Windows" */

/* One key per provider name. */
int credential_load(const char *provider, char *out, size_t cap); /* 1 if a saved key was copied */
int credential_save(const char *provider, const char *key);
int credential_delete(const char *provider); /* 1 if deleted or there was none */

#endif
