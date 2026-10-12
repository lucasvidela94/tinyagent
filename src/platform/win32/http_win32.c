#include "win32.h"

#include <winhttp.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "cancel_win32.h"
#include "config.h"
#include "console.h"
#include "http.h"
#include "httpwire.h"
#include "i18n.h"
#include "provider.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

static const char *winhttp_hint(DWORD e) {
  switch (e) {
  case ERROR_WINHTTP_NAME_NOT_RESOLVED: return N_("the server name did not resolve (DNS or no network)");
  case ERROR_WINHTTP_CANNOT_CONNECT: return N_("could not connect (network, proxy or firewall)");
  case ERROR_WINHTTP_TIMEOUT: return N_("timed out");
  case ERROR_WINHTTP_CONNECTION_ERROR: return N_("connection dropped; on Windows 7 this is usually TLS 1.2 not enabled");
  case ERROR_WINHTTP_SECURE_FAILURE:
  case ERROR_WINHTTP_SECURE_CHANNEL_ERROR: return N_("TLS failed: check TLS 1.2 (KB3140245), root certificates and the date and time");
  case ERROR_WINHTTP_SECURE_INVALID_CA: return N_("untrusted certificate: update the root certificates");
  case ERROR_WINHTTP_SECURE_CERT_DATE_INVALID: return N_("certificate out of date: check the Windows date and time");
  case ERROR_WINHTTP_SECURE_CERT_CN_INVALID: return N_("the certificate does not match the host (an intercepting proxy?)");
  default: return N_("network error");
  }
}

/* Windows 7 WinHTTP defaults to SSL3/TLS 1.0; ask for TLS 1.1/1.2 explicitly.
 * Needs KB3140245, but no DefaultSecureProtocols registry change. */
static void enable_tls12(HINTERNET ses) {
  static int warned;
  DWORD proto = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
  if (!WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &proto, sizeof proto) && !warned++)
    say(tr("\nWarning: WinHTTP refused to enable TLS 1.2 (error %lu). On Windows 7 SP1, install KB3140245.\n"),
        (unsigned long)GetLastError());
}

static wchar_t *auth_headers(const char *api_key, const char *session_header, const char *session_id,
                             size_t *wbytes) {
  Buf h = {0};
  char *s;
  wchar_t *w;
  int n;
  buf_str(&h, "Content-Type: application/json\r\n");
  if (*api_key) { /* local servers often need no key */
    buf_str(&h, "Authorization: Bearer ");
    buf_str(&h, api_key);
    buf_str(&h, "\r\n");
  }
  if (session_header && *session_header && session_id && *session_id) { /* e.g. OpenCode Go routing */
    buf_str(&h, session_header);
    buf_str(&h, ": ");
    buf_str(&h, session_id);
    buf_str(&h, "\r\n");
  }
  s = buf_take(&h);
  n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (n <= 0) n = 1;
  w = (wchar_t *)xrealloc(NULL, (size_t)n * sizeof(wchar_t));
  if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n)) w[0] = 0;
  SecureZeroMemory(s, strlen(s));
  free(s);
  *wbytes = (size_t)n * sizeof(wchar_t);
  return w;
}

static unsigned long last_status; /* 0 when no HTTP response arrived */
static unsigned long last_error;  /* WinHTTP error of the last failed request, 0 if none */
static int quiet;                 /* telemetry: print nothing, give up fast */

unsigned long http_last_status(void) { return last_status; }
unsigned long http_last_error(void) { return last_error; }

typedef struct {
  wchar_t host[256], path[1024];
  INTERNET_PORT port;
  int secure;
} Target;

/* Splits an http(s) URL with WinHttpCrackUrl; the query string stays part of the path. */
static int crack(const char *url, Target *t) {
  wchar_t wurl[1024], extra[512];
  URL_COMPONENTS u;
  if (!MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 1024)) return 0;
  ZeroMemory(&u, sizeof u);
  u.dwStructSize = sizeof u;
  u.lpszHostName = t->host;
  u.dwHostNameLength = 256;
  u.lpszUrlPath = t->path;
  u.dwUrlPathLength = 1024;
  u.lpszExtraInfo = extra;
  u.dwExtraInfoLength = 512;
  if (!WinHttpCrackUrl(wurl, 0, 0, &u)) return 0;
  if (u.nScheme != INTERNET_SCHEME_HTTPS && u.nScheme != INTERNET_SCHEME_HTTP) return 0;
  if (!t->path[0]) lstrcpyW(t->path, L"/");
  if (extra[0] && lstrlenW(t->path) + lstrlenW(extra) < 1024) lstrcatW(t->path, extra);
  t->port = u.nPort;
  t->secure = u.nScheme == INTERNET_SCHEME_HTTPS;
  return 1;
}

typedef struct {
  const char *label;
  const wchar_t *method;
  const char *api_key, *session_header, *session_id, *body; /* any may be NULL */
  size_t max;
  HttpChunk on_chunk; /* if set, a 2xx body is handed over as it arrives */
  void *ctx;
} Req;

/* Reads the body (into b, or to on_chunk for a 2xx stream), refusing more than max
 * bytes. 0 on WinHTTP error or overflow. */
static int read_body(HINTERNET h, const Req *r, int ok, Buf *b) {
  char chunk[8192];
  DWORD got = 0;
  size_t total = 0;
  for (;;) {
    if (!WinHttpReadData(h, chunk, sizeof chunk, &got)) return 0;
    if (!got) return 1;
    if ((total += got) > r->max) {
      if (!quiet) say(tr("\nThe response is over %lu bytes; it was discarded.\n"), (unsigned long)r->max);
      SetLastError(0);
      return 0;
    }
    if (ok && r->on_chunk) r->on_chunk(r->ctx, chunk, got);
    else buf_add(b, chunk, got);
  }
}

/* One request; redirects are followed. Returns the body (or "" when streamed) on 2xx. */
static char *request(const Target *t, const Req *r, size_t *out_len) {
  HINTERNET ses = NULL, con = NULL, req = NULL;
  const char *stage = N_("open a session");
  wchar_t *hdr = NULL;
  size_t hdr_bytes = 0;
  DWORD status = 0, sz = sizeof status, len = r->body ? (DWORD)strlen(r->body) : 0, err;
  Buf b = {0};
  char *out = NULL;
  int ok;

  last_status = last_error = 0;
  ses = WinHttpOpen(L"TinyShell/" TINYSHELL_VERSION_W, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);
  if (!ses) goto fail;
  enable_tls12(ses);
  if (quiet) WinHttpSetTimeouts(ses, 3000, 3000, 4000, 4000);
  else WinHttpSetTimeouts(ses, 15000, 15000, 30000, 120000);
  stage = N_("connect");
  if (!(con = WinHttpConnect(ses, t->host, t->port, 0))) goto fail;
  if (!(req = WinHttpOpenRequest(con, r->method, t->path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 t->secure ? WINHTTP_FLAG_SECURE : 0)))
    goto fail;
  cancel_watch_http(req); /* Ctrl+C closes it from the console handler thread */
  if (r->api_key || r->body)
    hdr = auth_headers(r->api_key ? r->api_key : "", r->session_header, r->session_id, &hdr_bytes);
  stage = N_("send");
  if (!WinHttpSendRequest(req, hdr ? hdr : WINHTTP_NO_ADDITIONAL_HEADERS, hdr ? (DWORD)-1L : 0,
                          r->body ? (LPVOID)r->body : WINHTTP_NO_REQUEST_DATA, len, len, 0))
    goto fail;
  stage = N_("receive");
  if (!WinHttpReceiveResponse(req, NULL)) goto fail;
  WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                      &status, &sz, WINHTTP_NO_HEADER_INDEX);
  last_status = status;
  ok = status >= 200 && status < 300;
  if (!read_body(req, r, ok, &b)) goto fail;
  if (ok) {
    if (out_len) *out_len = b.len;
    out = buf_take(&b);
  } else {
    if (b.len > 300) b.p[300] = 0;
    if (*http_status_hint(status) && status < 500) b.len = 0; /* the hint says it; skip the raw JSON */
    if (!quiet)
      say(tr("\n%s answered HTTP %lu%s%s%s\n"), tr(r->label), (unsigned long)status, tr(http_status_hint(status)),
          b.len ? ": " : "", b.len ? b.p : "");
  }
  goto done;

fail:
  last_error = err = GetLastError();
  if (err && !quiet && !cancel_requested())
    say(tr("\nCould not reach %s (%s): error %lu, %s.\n"), tr(r->label), tr(stage), (unsigned long)err,
        tr(winhttp_hint(err)));
done:
  free(b.p);
  if (hdr) {
    SecureZeroMemory(hdr, hdr_bytes);
    free(hdr);
  }
  if (req && cancel_release_http(req)) WinHttpCloseHandle(req);
  if (con) WinHttpCloseHandle(con);
  if (ses) WinHttpCloseHandle(ses);
  return out;
}

int http_post_stream(const Provider *p, const char *body, const char *api_key, const char *session_id,
                     HttpChunk on_chunk, void *ctx) {
  Target t;
  Req r;
  char *out;
  last_status = last_error = 0;
  if (!crack(p->url, &t)) {
    say(tr("\nInvalid provider URL: %s\n"), p->url);
    return 0;
  }
  memset(&r, 0, sizeof r);
  r.label = p->name;
  r.method = L"POST";
  r.api_key = api_key;
  r.session_header = p->session_header;
  r.session_id = session_id;
  r.body = body;
  r.max = 4 * MAX_HTTP; /* a long streamed answer carries per-event overhead */
  r.on_chunk = on_chunk;
  r.ctx = ctx;
  out = request(&t, &r, NULL);
  free(out);
  return out != NULL;
}

char *http_get(const char *url, const char *api_key, size_t max, size_t *len) {
  Target t;
  Req r;
  last_status = last_error = 0;
  if (!crack(url, &t)) {
    say(tr("\nInvalid URL: %s\n"), url);
    return NULL;
  }
  memset(&r, 0, sizeof r);
  r.label = t.secure ? N_("the server") : N_("the server (http)");
  r.method = L"GET";
  r.api_key = api_key && *api_key ? api_key : NULL;
  r.max = max;
  return request(&t, &r, len);
}

char *http_post_json(const char *url, const char *body, int quiet_mode, size_t max) {
  Target t;
  Req r;
  char *out;
  last_status = last_error = 0;
  if (!crack(url, &t)) return NULL;
  memset(&r, 0, sizeof r);
  r.label = N_("the report server");
  r.method = L"POST";
  r.body = body;
  r.max = max;
  quiet = quiet_mode;
  out = request(&t, &r, NULL);
  quiet = 0;
  return out;
}
