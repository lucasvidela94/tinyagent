/* `tinyshell login`: pick a provider, paste its API key (shown as asterisks), pick a model
 * from the provider's live list, and save: the key in the Credential Manager, the provider in config.ini (Win32 adapter).
 * Running login is the user's explicit request to save, so it does not ask again. */
#ifndef TINYSHELL_LOGIN_H
#define TINYSHELL_LOGIN_H

#include <stddef.h>

#include "provider.h"

/* Returns 1 and fills p and key (if non-NULL) on success, 0 if cancelled or not saved. */
int login_run(Provider *p, char *key, size_t cap);

/* Fetches the provider's model list (<base>/models, with key if any) and lets the user
 * pick one with filtering. Sets p->model and returns 1 if a model was chosen. */
int login_pick_model(Provider *p, const char *key);

#endif
