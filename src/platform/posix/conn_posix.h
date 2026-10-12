/* One TCP connection, plain or TLS (Mbed TLS), for the POSIX HTTP adapter (private to
 * posix/). Every wait is bounded and Ctrl+C shuts the socket down (cancel_posix.h). */
#ifndef TINYSHELL_CONN_POSIX_H
#define TINYSHELL_CONN_POSIX_H

#include <stddef.h>

#include "url.h"

typedef struct Conn Conn;

/* What went wrong, for the message the user sees and for telemetry codes. */
enum {
  CONN_OK,
  CONN_DNS,          /* the name did not resolve */
  CONN_CONNECT,      /* refused or unreachable */
  CONN_TIMEOUT,
  CONN_TLS,          /* handshake failed */
  CONN_CERT_DATE,    /* certificate expired or not yet valid: usually the PC's clock */
  CONN_CERT_UNTRUSTED,
  CONN_CERT_NAME,    /* certificate is for another host (an intercepting proxy?) */
  CONN_IO            /* the connection broke */
};

/* timeout_ms bounds connecting and each read or write. NULL on failure, with *error set. */
Conn *conn_open(const Url *u, int timeout_ms, int *error);
int conn_write(Conn *c, const char *data, size_t n); /* 1 if all was sent */
/* Bytes read (> 0), 0 at the end of the stream, -1 on error or timeout (conn_error says). */
long conn_read(Conn *c, char *buf, size_t n);
int conn_error(const Conn *c);
void conn_close(Conn *c);

const char *conn_hint(int error); /* English (show it through tr()), for "Could not reach ...: <hint>" */

#endif
