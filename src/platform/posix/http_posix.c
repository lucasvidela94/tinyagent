/* HTTP(S) on Linux: core/url and core/httpwire over conn_posix (TCP + Mbed TLS). Same
 * contract and messages as the WinHTTP adapter. */
#include "posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "config.h"
#include "conn_posix.h"
#include "console.h"
#include "http.h"
#include "httpwire.h"
#include "i18n.h"
#include "url.h"
#include "util.h"

#define MAX_REDIRECTS 5

static unsigned long last_status, last_error;

unsigned long http_last_status(void) { return last_status; }
unsigned long http_last_error(void) { return last_error; }

typedef struct {
  const char *label, *method, *api_key, *session_header, *session_id, *body;
  size_t max;
  int quiet;
  HttpChunk on_chunk; /* if set, a 2xx body is handed over as it arrives */
  void *ctx;
} Req;

typedef struct {
  const Req *r;
  HttpwireResp resp;
  Buf body;
  size_t total;
  int overflow;
} Rx;

static void on_body(void *ctx, const char *data, size_t n) {
  Rx *x = (Rx *)ctx;
  int ok = x->resp.status >= 200 && x->resp.status < 300;
  if (x->overflow || (x->total += n) > x->r->max) {
    x->overflow = 1;
    return;
  }
  if (ok && x->r->on_chunk) x->r->on_chunk(x->r->ctx, data, n);
  else buf_add(&x->body, data, n);
}

static void fail(const Req *r, const char *stage, int error) {
  last_error = (unsigned long)error;
  if (!r->quiet && !cancel_requested())
    say(tr("\nCould not reach %s (%s): %s.\n"), tr(r->label), tr(stage), tr(conn_hint(error)));
}

/* One exchange. Returns 1 when a full response arrived (any status); x holds it. */
static int exchange(const Url *u, const Req *r, Rx *x) {
  Buf out = {0};
  char chunk[8192];
  int error = CONN_OK, st = HTTPWIRE_MORE;
  Conn *c = conn_open(u, r->quiet ? 4000 : 30000, &error);
  if (!c) {
    fail(r, N_("connect"), error);
    return 0;
  }
  httpwire_request(&out, r->method, u, r->api_key, r->session_header, r->session_id, r->body);
  if (!conn_write(c, out.p, out.len)) {
    wipe(out.p, out.len); /* holds the API key */
    free(out.p);
    fail(r, N_("send"), conn_error(c));
    conn_close(c);
    return 0;
  }
  wipe(out.p, out.len);
  free(out.p);
  while (st == HTTPWIRE_MORE) {
    long n = conn_read(c, chunk, sizeof chunk);
    if (n > 0) st = httpwire_feed(&x->resp, chunk, (size_t)n, on_body, x);
    else if (n == 0) st = httpwire_eof(&x->resp);
    else break;
  }
  if (st != HTTPWIRE_DONE) {
    fail(r, N_("receive"), st == HTTPWIRE_BAD ? CONN_IO : conn_error(c));
    conn_close(c);
    return 0;
  }
  conn_close(c);
  return 1;
}

/* Follows redirects; returns the 2xx body ("" when streamed) or NULL after a message. */
static char *request(const char *url, const Req *orig, size_t *out_len) {
  Url u;
  Req req = *orig, *r = &req;
  int hops;
  last_status = last_error = 0;
  if (!url_parse(url, &u)) {
    if (!r->quiet) say(tr("\nInvalid URL: %s\n"), url);
    return NULL;
  }
  for (hops = 0; hops <= MAX_REDIRECTS; hops++) {
    Rx x;
    char *out = NULL;
    int ok, status;
    memset(&x, 0, sizeof x);
    x.r = r;
    httpwire_init(&x.resp);
    if (!exchange(&u, r, &x)) {
      httpwire_free(&x.resp);
      free(x.body.p);
      return NULL;
    }
    status = x.resp.status;
    last_status = (unsigned long)status;
    if (status >= 300 && status < 400 && x.resp.location[0] && !strcmp(r->method, "GET")) {
      int https = u.https;
      char host[sizeof u.host];
      memcpy(host, u.host, sizeof host);
      if (!url_parse(x.resp.location, &u) || (https && !u.https)) { /* never downgrade to http */
        if (!r->quiet) say(tr("\nInvalid redirect from %s.\n"), tr(r->label));
        httpwire_free(&x.resp);
        free(x.body.p);
        return NULL;
      }
      if (strcmp(host, u.host)) req.api_key = NULL; /* the key is for the first host only */
      httpwire_free(&x.resp);
      free(x.body.p);
      continue;
    }
    ok = status >= 200 && status < 300;
    if (x.overflow) {
      if (!r->quiet) say(tr("\nThe response is over %lu bytes; it was discarded.\n"), (unsigned long)r->max);
    } else if (ok) {
      if (out_len) *out_len = x.body.len;
      out = x.body.p ? buf_take(&x.body) : xstrdup("");
    } else if (!r->quiet) {
      if (x.body.len > 300) x.body.p[300] = 0;
      if (*http_status_hint((unsigned long)status) && status < 500) x.body.len = 0; /* the hint says it */
      say(tr("\n%s answered HTTP %d%s%s%s\n"), tr(r->label), status, tr(http_status_hint((unsigned long)status)),
          x.body.len ? ": " : "", x.body.len ? x.body.p : "");
    }
    httpwire_free(&x.resp);
    free(x.body.p);
    return out;
  }
  if (!r->quiet) say(tr("\nToo many redirects from %s.\n"), tr(r->label));
  return NULL;
}

int http_post_stream(const Provider *p, const char *body, const char *api_key, const char *session_id,
                     HttpChunk on_chunk, void *ctx) {
  Req r;
  char *out;
  memset(&r, 0, sizeof r);
  r.label = p->name;
  r.method = "POST";
  r.api_key = api_key;
  r.session_header = p->session_header;
  r.session_id = session_id;
  r.body = body;
  r.max = 4 * MAX_HTTP; /* a long streamed answer carries per-event overhead */
  r.on_chunk = on_chunk;
  r.ctx = ctx;
  out = request(p->url, &r, NULL);
  free(out);
  return out != NULL;
}

char *http_get(const char *url, const char *api_key, size_t max, size_t *len) {
  Req r;
  memset(&r, 0, sizeof r);
  r.label = strncmp(url, "https://", 8) ? N_("the server (http)") : N_("the server");
  r.method = "GET";
  r.api_key = api_key;
  r.max = max;
  return request(url, &r, len);
}

char *http_post_json(const char *url, const char *body, int quiet, size_t max) {
  Req r;
  memset(&r, 0, sizeof r);
  r.label = N_("the report server");
  r.method = "POST";
  r.body = body;
  r.max = max;
  r.quiet = quiet;
  return request(url, &r, NULL);
}
