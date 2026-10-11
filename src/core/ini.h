/* Reading and editing INI text ("[section]" and "key = value" lines) the way TinyShell's
 * config.ini is used: comments (; or #) and unknown lines are kept untouched. Section and
 * key names match ignoring ASCII case, like the Windows profile API. Portable C; the
 * POSIX settings adapter uses it (Windows has the profile API). */
#ifndef TINYSHELL_INI_H
#define TINYSHELL_INI_H

#include <stddef.h>

#include "util.h"

/* Copies the value (spaces around it trimmed) into out. 1 if the key exists, even when
 * its value is empty; 0 if it does not or the value does not fit. */
int ini_get(const char *text, const char *section, const char *key, char *out, size_t cap);

/* Appends to out the text with key set to value: the existing line is replaced, or a line
 * is added at the end of the section, or the section is added at the end. 0 (nothing
 * appended) if a name or the value contains a line break, or a name contains '=' or ']'. */
int ini_set(const char *text, const char *section, const char *key, const char *value, Buf *out);

#endif
