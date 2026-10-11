/* User settings in %LOCALAPPDATA%\TinyShell\config.ini (Win32 adapter). Plain text,
 * editable with Notepad, never holds secrets. Read with GetPrivateProfileString. */
#ifndef TINYSHELL_SETTINGS_H
#define TINYSHELL_SETTINGS_H

#include "provider.h"

/* Defaults, then the [provider] section of config.ini. Creates a commented config.ini
 * on first run. Invalid values are reported and ignored (the default stays). */
void settings_load(Provider *p);

int settings_save_model(const char *model);  /* writes [provider] model=, 1 on success */
int settings_save_provider(const Provider *p); /* writes the whole [provider] section */
const char *settings_path(void);            /* UTF-8, for messages */

#endif
