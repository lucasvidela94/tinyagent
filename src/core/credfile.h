/* The Linux API key file (~/.config/tinyshell/credentials, mode 0600): one
 * "<provider>\t<key>" line per provider. Portable C, so the format is host-tested;
 * src/platform/posix/credential_posix.c reads and writes the file. */
#ifndef TINYSHELL_CREDFILE_H
#define TINYSHELL_CREDFILE_H

#include <stddef.h>

#include "util.h"

/* Copies the provider's key from the file's text into out. 1 if found and it fits. */
int credfile_get(const char *text, const char *provider, char *out, size_t cap);

/* Appends to out the file's text with the provider's line replaced by `key`, or removed
 * when key is NULL. Other providers' lines are kept. 0 (and nothing appended) if the
 * provider or key contains a tab or a line break. */
int credfile_set(const char *text, const char *provider, const char *key, Buf *out);

#endif
