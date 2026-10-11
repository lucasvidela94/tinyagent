/* HTTPS requests over WinHTTP (Win32 adapter): the model API and release downloads. */
#ifndef TINYSHELL_HTTP_H
#define TINYSHELL_HTTP_H

#include <stddef.h>

/* Returns the 2xx response body (at most MAX_HTTP bytes; caller frees), or NULL after
 * printing a readable error: WinHTTP code with a hint, or HTTP status with the start of
 * the body. The API key is only used for the Authorization header and is never printed. */
char *http_post(const char *body, const char *api_key);

/* GETs an https:// URL (redirects followed), refusing bodies over max bytes. Returns the
 * body (binary-safe, NUL-terminated for convenience) and its length in *len, or NULL
 * after printing an error. Caller frees. */
char *http_get(const char *url, size_t max, size_t *len);

/* HTTP status of the last request (e.g. 401 for a rejected API key), 0 if none arrived. */
unsigned long http_last_status(void);

#endif
