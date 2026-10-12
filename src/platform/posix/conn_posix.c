#include "posix.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <psa/crypto.h>

#include "ca_bundle.h"
#include "cancel.h"
#include "cancel_posix.h"
#include "conn_posix.h"

struct Conn {
  int fd, tls, timeout_ms, err;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
};

/* Where distros keep their bundle (Debian, Fedora/RHEL, openSUSE, Alpine/others). */
static const char *const SYSTEM_BUNDLES[] = {"/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt",
                                             "/etc/ssl/ca-bundle.pem", "/etc/ssl/cert.pem"};

static mbedtls_x509_crt roots;
static int roots_ready;

/* The system's roots (may be old) plus Mozilla's embedded ones: either may complete a chain. */
static int load_roots(void) {
  size_t i;
  int ok = 0;
  if (roots_ready) return 1;
  if (psa_crypto_init() != PSA_SUCCESS) return 0;
  mbedtls_x509_crt_init(&roots);
  for (i = 0; i < sizeof SYSTEM_BUNDLES / sizeof SYSTEM_BUNDLES[0]; i++)
    if (mbedtls_x509_crt_parse_file(&roots, SYSTEM_BUNDLES[i]) >= 0) break; /* > 0: some entries skipped */
  for (i = 0; i < (size_t)CA_BUNDLE_COUNT; i++)
    if (mbedtls_x509_crt_parse(&roots, (const unsigned char *)CA_BUNDLE[i], strlen(CA_BUNDLE[i]) + 1) == 0) ok = 1;
  roots_ready = ok;
  return ok;
}

/* Waits until fd is ready for `events`; sets c->err and returns 0 on timeout or Ctrl+C. */
static int wait_fd(Conn *c, short events) {
  struct pollfd p;
  int r;
  p.fd = c->fd;
  p.events = events;
  do r = poll(&p, 1, c->timeout_ms);
  while (r < 0 && errno == EINTR && !cancel_requested());
  if (r > 0) return 1;
  c->err = r == 0 ? CONN_TIMEOUT : CONN_IO;
  return 0;
}

static int tls_send(void *ctx, const unsigned char *buf, size_t n) {
  Conn *c = (Conn *)ctx;
  ssize_t w;
  if (!wait_fd(c, POLLOUT)) return MBEDTLS_ERR_NET_SEND_FAILED;
  w = send(c->fd, buf, n, 0);
  if (w > 0) return (int)w;
  c->err = CONN_IO;
  return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int tls_recv(void *ctx, unsigned char *buf, size_t n) {
  Conn *c = (Conn *)ctx;
  ssize_t r;
  if (!wait_fd(c, POLLIN)) return MBEDTLS_ERR_NET_RECV_FAILED;
  r = recv(c->fd, buf, n, 0);
  if (r >= 0) return (int)r;
  c->err = CONN_IO;
  return MBEDTLS_ERR_NET_RECV_FAILED;
}

/* Non-blocking connect to the first address that answers within the timeout. */
static int tcp_connect(Conn *c, const Url *u) {
  struct addrinfo hints, *res = NULL, *a;
  char port[8];
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  snprintf(port, sizeof port, "%u", u->port);
  if (getaddrinfo(u->host, port, &hints, &res) != 0) {
    c->err = CONN_DNS;
    return 0;
  }
  c->err = CONN_CONNECT;
  for (a = res; a && c->fd < 0; a = a->ai_next) {
    int fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol), soerr = 0;
    socklen_t len = sizeof soerr;
    if (fd < 0) continue;
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    c->fd = fd;
    cancel_watch_socket(fd);
    if (connect(fd, a->ai_addr, a->ai_addrlen) == 0 ||
        (errno == EINPROGRESS && wait_fd(c, POLLOUT) && !getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len) && !soerr)) {
      c->err = CONN_OK;
      break;
    }
    cancel_release_socket();
    close(fd);
    c->fd = -1;
    if (c->err != CONN_TIMEOUT) c->err = CONN_CONNECT;
  }
  freeaddrinfo(res);
  return c->fd >= 0;
}

static int cert_error(const Conn *c) {
  uint32_t flags = mbedtls_ssl_get_verify_result(&c->ssl);
  if (flags == (uint32_t)-1 || flags == 0) return c->err != CONN_OK ? c->err : CONN_TLS;
  if (flags & (MBEDTLS_X509_BADCERT_EXPIRED | MBEDTLS_X509_BADCERT_FUTURE)) return CONN_CERT_DATE;
  if (flags & MBEDTLS_X509_BADCERT_CN_MISMATCH) return CONN_CERT_NAME;
  return CONN_CERT_UNTRUSTED;
}

static int tls_start(Conn *c, const Url *u) {
  int r;
  if (!load_roots()) return 0;
  mbedtls_ssl_init(&c->ssl);
  mbedtls_ssl_config_init(&c->conf);
  c->tls = 1;
  if (mbedtls_ssl_config_defaults(&c->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_PRESET_DEFAULT) != 0)
    return 0;
  mbedtls_ssl_conf_authmode(&c->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&c->conf, &roots, NULL);
  if (mbedtls_ssl_setup(&c->ssl, &c->conf) != 0 || mbedtls_ssl_set_hostname(&c->ssl, u->host) != 0) return 0;
  mbedtls_ssl_set_bio(&c->ssl, c, tls_send, tls_recv, NULL);
  do r = mbedtls_ssl_handshake(&c->ssl);
  while (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE);
  if (r == 0) return 1;
  c->err = cert_error(c);
  return 0;
}

Conn *conn_open(const Url *u, int timeout_ms, int *error) {
  Conn *c = (Conn *)calloc(1, sizeof *c);
  if (!c) return NULL;
  c->fd = -1;
  c->timeout_ms = timeout_ms;
  if (tcp_connect(c, u) && (!u->https || tls_start(c, u))) {
    *error = CONN_OK;
    return c;
  }
  *error = c->err != CONN_OK ? c->err : CONN_TLS;
  conn_close(c);
  return NULL;
}

int conn_write(Conn *c, const char *data, size_t n) {
  while (n) {
    long w;
    if (c->tls) {
      int r = mbedtls_ssl_write(&c->ssl, (const unsigned char *)data, n);
      if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
      w = r;
    } else {
      w = wait_fd(c, POLLOUT) ? (long)send(c->fd, data, n, 0) : -1;
    }
    if (w <= 0) {
      if (c->err == CONN_OK) c->err = CONN_IO;
      return 0;
    }
    data += w;
    n -= (size_t)w;
  }
  return 1;
}

long conn_read(Conn *c, char *buf, size_t n) {
  if (!c->tls) {
    long r = wait_fd(c, POLLIN) ? (long)recv(c->fd, buf, n, 0) : -1;
    if (r < 0 && c->err == CONN_OK) c->err = CONN_IO;
    return r;
  }
  for (;;) {
    int r = mbedtls_ssl_read(&c->ssl, (unsigned char *)buf, n);
    if (r > 0) return r;
    if (r == 0 || r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return 0;
    if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE ||
        r == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) /* TLS 1.3 sends tickets after the handshake */
      continue;
    if (c->err == CONN_OK) c->err = CONN_IO;
    return -1;
  }
}

int conn_error(const Conn *c) { return c->err; }

void conn_close(Conn *c) {
  if (!c) return;
  if (c->tls) {
    mbedtls_ssl_free(&c->ssl);
    mbedtls_ssl_config_free(&c->conf);
  }
  if (c->fd >= 0) {
    cancel_release_socket();
    close(c->fd);
  }
  free(c);
}

const char *conn_hint(int error) {
  switch (error) {
  case CONN_DNS: return "no se resolvió el servidor (DNS o sin red)";
  case CONN_CONNECT: return "no se pudo conectar (red, proxy o firewall)";
  case CONN_TIMEOUT: return "tiempo de espera agotado";
  case CONN_CERT_DATE: return "certificado fuera de fecha: revisá la fecha y hora del equipo (date)";
  case CONN_CERT_UNTRUSTED: return "certificado no confiable (¿un proxy que intercepta HTTPS?)";
  case CONN_CERT_NAME: return "el certificado no coincide con el servidor (¿un proxy que intercepta?)";
  case CONN_TLS: return "falló TLS al negociar la conexión segura";
  default: return "la conexión se cortó";
  }
}
