/* Allocation, growable byte buffer, UTF-8 checks and JSON string escaping. Portable C. */
#ifndef TINYSHELL_UTIL_H
#define TINYSHELL_UTIL_H

#include <stddef.h>

/* Exit with a message on out-of-memory; callers never see NULL. */
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

/* Growable NUL-terminated buffer. Zero-initialize ({0}) before use. */
typedef struct { char *p; size_t len, cap; } Buf;
void buf_add(Buf *b, const char *s, size_t n);
void buf_str(Buf *b, const char *s);
char *buf_take(Buf *b); /* hands ownership to the caller ("" if empty) and resets b */
/* Keeps at most `limit` bytes in b; whatever does not fit is counted in *dropped. */
void buf_add_capped(Buf *b, const char *s, size_t n, size_t limit, size_t *dropped);

void trim_line(char *s); /* strips trailing CR, LF, spaces and tabs */

int utf8_len(const unsigned char *s); /* length of the valid sequence at s, 0 if invalid */
int utf8_valid(const char *s);

/* Display columns of the first n bytes (one per code point; no East Asian wide forms). */
int utf8_cols(const char *s, size_t n);
/* Bytes of s that fit in cols columns, stopping before '\n' and, when a word would be cut,
 * at the last space. Always at least one code point unless s is empty or starts with '\n'. */
size_t text_fit(const char *s, int cols);

/* Escapes s for a JSON string body. Invalid UTF-8 becomes \\ufffd, so output is always valid. */
char *jesc(const char *s);

#endif
