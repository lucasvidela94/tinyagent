/* HTTP(S) requests over WinHTTP (Win32 adapter): the model API and release downloads. */
#ifndef TINYSHELL_HTTP_H
#define TINYSHELL_HTTP_H

#include <stddef.h>

#include "provider.h"

/* POSTs to the provider's chat-completions URL (http or https). session_id goes in the
 * provider's session header, if it has one (OpenCode Go rejects requests without
 * x-opencode-session). Returns the 2xx response body
 * (at most MAX_HTTP bytes; caller frees), or NULL after printing a readable error:
 * WinHTTP code with a hint, or HTTP status with the start of the body. The API key is only
 * used for the Authorization header and is never printed. */
char *http_post(const Provider *p, const char *body, const char *api_key, const char *session_id);

/* GETs an http(s) URL (redirects followed), with "Authorization: Bearer <api_key>" when
 * api_key is non-empty. Refuses bodies over max bytes. Returns the body (binary-safe,
 * NUL-terminated for convenience) and its length in *len, or NULL after printing an
 * error. Caller frees. Callers that need TLS must pass an https:// URL. */
char *http_get(const char *url, const char *api_key, size_t max, size_t *len);

/* POSTs a JSON body (no API key) to an http(s) URL; used for reports and telemetry.
 * quiet = 1: prints nothing and gives up after a few seconds, for background-free
 * telemetry that must never slow the user down. Returns the 2xx body or NULL. */
char *http_post_json(const char *url, const char *body, int quiet, size_t max);

unsigned long http_last_error(void); /* WinHTTP error code of the last failure, 0 if none */

/* HTTP status of the last request (e.g. 401 for a rejected API key), 0 if none arrived. */
unsigned long http_last_status(void);

#endif
