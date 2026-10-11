/* HTTPS POST to the configured API endpoint over WinHTTP (Win32 adapter). */
#ifndef TINYSHELL_HTTP_H
#define TINYSHELL_HTTP_H

/* Returns the 2xx response body (at most MAX_HTTP bytes; caller frees), or NULL after
 * printing a readable error: WinHTTP code with a hint, or HTTP status with the start of
 * the body. The API key is only used for the Authorization header and is never printed. */
char *http_post(const char *body, const char *api_key);

#endif
