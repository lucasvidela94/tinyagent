/* Text for terminals that are not UTF-8. Portable C.
 * Old Linux distros often run with LANG=C (ASCII) or a Latin-1 locale; what such a terminal
 * cannot show is replaced, never sent as raw UTF-8 bytes (which would print as garbage). */
#ifndef TINYSHELL_CHARSET_H
#define TINYSHELL_CHARSET_H

#include "util.h"

enum { CHARSET_UTF8, CHARSET_LATIN1, CHARSET_ASCII };

/* Charset named by a locale string such as $LC_ALL/$LC_CTYPE/$LANG ("es_AR.UTF-8",
 * "es_ES@euro", "C"). NULL or empty means ASCII. */
int charset_from_locale(const char *locale);

/* Appends utf8 converted for `charset`. Latin-1 keeps U+0000-U+00FF; ASCII turns accented
 * Latin letters into their base letter ("ñ" -> "n", "¿" -> "?"). Anything else uses
 * ascii_fallback() or '?'. Invalid UTF-8 bytes become '?'. */
void charset_encode(const char *utf8, int charset, Buf *out);

/* Appends text typed in `charset` as UTF-8 (Latin-1 bytes become two-byte sequences;
 * for ASCII and UTF-8 the bytes are copied). */
void charset_decode(const char *text, size_t n, int charset, Buf *out);

#endif
