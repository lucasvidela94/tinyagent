/* API key storage in the Windows Credential Manager (Win32 adapter). Opt-in only.
 * Windows encrypts it with the user's logon (DPAPI): other accounts on the PC cannot
 * read it. Visible and removable in Control Panel > Credential Manager, or --logout. */
#ifndef TINYSHELL_CREDENTIAL_H
#define TINYSHELL_CREDENTIAL_H

#include <stddef.h>

/* One key per provider name, stored as "TinyShell/<provider>". */
int credential_load(const char *provider, char *out, size_t cap); /* 1 if a saved key was copied */
int credential_save(const char *provider, const char *key);
int credential_delete(const char *provider); /* 1 if deleted or there was none */

#endif
