/* HTTP/1.1 on the wire for the POSIX adapter (WinHTTP does this on Windows): the request
 * text and an incremental response parser that hands the body over as it arrives, which
 * streaming needs. Portable C; fuzzed, since it reads what the network sends. */
#ifndef TINYSHELL_HTTPWIRE_H
#define TINYSHELL_HTTPWIRE_H

#include <stddef.h>

#include "url.h"
#include "util.h"

/* Appends "<method> <path> HTTP/1.1" with Host, User-Agent, Connection: close, the
 * Content-Type/Length for a body, Authorization when api_key is non-empty, and the
 * session header when both its name and value are non-empty; then the body. */
void httpwire_request(Buf *out, const char *method, const Url *u, const char *api_key, const char *session_header,
                      const char *session_id, const char *body);

typedef void (*HttpwireBody)(void *ctx, const char *data, size_t n);

typedef struct {
  int state, status, chunked;
  long long left;   /* bytes left in the body or the current chunk; -1: until close */
  Buf line;         /* the head, or a chunk-size line, being collected */
  char location[2048];
} HttpwireResp;

enum { HTTPWIRE_MORE, HTTPWIRE_DONE, HTTPWIRE_BAD };

void httpwire_init(HttpwireResp *r);
void httpwire_free(HttpwireResp *r);

/* Feeds bytes from the connection. Body bytes (de-chunked) go to on_body as they arrive.
 * Returns HTTPWIRE_DONE once the body is complete, HTTPWIRE_BAD for a malformed or
 * oversized head or chunk line. After the head, r->status and r->location are set. */
int httpwire_feed(HttpwireResp *r, const char *data, size_t n, HttpwireBody on_body, void *ctx);

/* Call at connection close: DONE if the body was complete or ran until close. */
int httpwire_eof(const HttpwireResp *r);

/* " (API key inválida o sin acceso al modelo)" and the like, or "" (shared with Windows). */
const char *http_status_hint(unsigned long status);

#endif
