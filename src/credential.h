/* API key storage in the Windows Credential Manager (Win32 adapter). Opt-in only.
 * Windows encrypts it with the user's logon (DPAPI): other accounts on the PC cannot
 * read it. Visible and removable in Control Panel > Credential Manager, or --logout. */
#ifndef TINYSHELL_CREDENTIAL_H
#define TINYSHELL_CREDENTIAL_H

#include <stddef.h>

int credential_load(char *out, size_t cap); /* 1 if a saved key was copied into out */
int credential_save(const char *key);
int credential_delete(void); /* 1 if deleted or there was none */

#endif
