#include "win32.h"

#include <winhttp.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "provider.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

static const char *winhttp_hint(DWORD e) {
  switch (e) {
  case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "no se resolvió el servidor (DNS o sin red)";
  case ERROR_WINHTTP_CANNOT_CONNECT: return "no se pudo conectar (red, proxy o firewall)";
  case ERROR_WINHTTP_TIMEOUT: return "tiempo de espera agotado";
  case ERROR_WINHTTP_CONNECTION_ERROR: return "conexión cortada; en Windows 7 suele ser TLS 1.2 no habilitado";
  case ERROR_WINHTTP_SECURE_FAILURE:
  case ERROR_WINHTTP_SECURE_CHANNEL_ERROR: return "falló TLS: revisá TLS 1.2 (KB3140245), certificados raíz y fecha/hora";
  case ERROR_WINHTTP_SECURE_INVALID_CA: return "certificado no confiable: actualizá los certificados raíz";
  case ERROR_WINHTTP_SECURE_CERT_DATE_INVALID: return "certificado fuera de fecha: revisá la fecha/hora de Windows";
  case ERROR_WINHTTP_SECURE_CERT_CN_INVALID: return "el certificado no coincide con el host (¿proxy que intercepta?)";
  default: return "error de red";
  }
}

static const char *status_hint(DWORD status) {
  if (status == 401 || status == 403) return " (API key inválida o sin acceso al modelo)";
  if (status == 404) return " (no encontrado; ¿hay un release publicado?)";
  if (status == 429) return " (límite de uso; esperá y reintentá)";
  if (status >= 500) return " (problema del proveedor; reintentá más tarde)";
  return "";
}

/* Windows 7 WinHTTP defaults to SSL3/TLS 1.0; ask for TLS 1.1/1.2 explicitly.
 * Needs KB3140245, but no DefaultSecureProtocols registry change. */
static void enable_tls12(HINTERNET ses) {
  static int warned;
  DWORD proto = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
  if (!WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &proto, sizeof proto) && !warned++)
    say("\nAviso: WinHTTP no aceptó habilitar TLS 1.2 (error %lu). En Windows 7 SP1 instalá KB3140245.\n",
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

/* Reads the body into b, refusing more than max bytes. 0 on WinHTTP error or overflow. */
static int read_body(HINTERNET req, Buf *b, size_t max) {
  char chunk[8192];
  DWORD got = 0;
  for (;;) {
    if (!WinHttpReadData(req, chunk, sizeof chunk, &got)) return 0;
    if (!got) return 1;
    if (b->len + got > max) {
      if (!quiet) say("\nLa respuesta supera %lu bytes; se descartó.\n", (unsigned long)max);
      SetLastError(0);
      return 0;
    }
    buf_add(b, chunk, got);
  }
}


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

/* One request. api_key and body may be NULL (plain GET). Redirects are followed. */
static char *request(const char *label, const Target *t, const wchar_t *method, const char *api_key,
                     const char *session_header, const char *session_id, const char *body, size_t max,
                     size_t *out_len) {
  HINTERNET ses = NULL, con = NULL, req = NULL;
  const char *stage = "abrir sesión";
  wchar_t *hdr = NULL;
  size_t hdr_bytes = 0;
  DWORD status = 0, sz = sizeof status, len = body ? (DWORD)strlen(body) : 0, err;
  Buf b = {0};
  char *out = NULL;

  last_status = last_error = 0;
  ses = WinHttpOpen(L"TinyShell/" TINYSHELL_VERSION_W, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);
  if (!ses) goto fail;
  enable_tls12(ses);
  if (quiet) WinHttpSetTimeouts(ses, 3000, 3000, 4000, 4000);
  else WinHttpSetTimeouts(ses, 15000, 15000, 30000, 120000);
  stage = "conectar";
  if (!(con = WinHttpConnect(ses, t->host, t->port, 0))) goto fail;
  if (!(req = WinHttpOpenRequest(con, method, t->path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 t->secure ? WINHTTP_FLAG_SECURE : 0)))
    goto fail;
  if (api_key || body) hdr = auth_headers(api_key ? api_key : "", session_header, session_id, &hdr_bytes);
  stage = "enviar";
  if (!WinHttpSendRequest(req, hdr ? hdr : WINHTTP_NO_ADDITIONAL_HEADERS, hdr ? (DWORD)-1L : 0,
                          body ? (LPVOID)body : WINHTTP_NO_REQUEST_DATA, len, len, 0))
    goto fail;
  stage = "recibir";
  if (!WinHttpReceiveResponse(req, NULL)) goto fail;
  WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                      &status, &sz, WINHTTP_NO_HEADER_INDEX);
  last_status = status;
  if (!read_body(req, &b, max)) goto fail;
  if (status >= 200 && status < 300) {
    if (out_len) *out_len = b.len;
    out = buf_take(&b);
  } else {
    if (b.len > 300) b.p[300] = 0;
    if (*status_hint(status) && status < 500) b.len = 0; /* the hint says it; skip the raw JSON */
    if (!quiet) say("\n%s respondió HTTP %lu%s%s%s\n", label, (unsigned long)status, status_hint(status),
        b.len ? ": " : "", b.len ? b.p : "");
  }
  goto done;

fail:
  last_error = err = GetLastError();
  if (err && !quiet) say("\nNo se pudo contactar %s (%s): error %lu, %s.\n", label, stage, (unsigned long)err, winhttp_hint(err));
done:
  free(b.p);
  if (hdr) {
    SecureZeroMemory(hdr, hdr_bytes);
    free(hdr);
  }
  if (req) WinHttpCloseHandle(req);
  if (con) WinHttpCloseHandle(con);
  if (ses) WinHttpCloseHandle(ses);
  return out;
}

char *http_post(const Provider *p, const char *body, const char *api_key, const char *session_id) {
  Target t;
  last_status = 0;
  if (!crack(p->url, &t)) {
    say("\nURL del proveedor inválida: %s\n", p->url);
    return NULL;
  }
  return request(p->name, &t, L"POST", api_key, p->session_header, session_id, body, MAX_HTTP, NULL);
}

char *http_get(const char *url, const char *api_key, size_t max, size_t *len) {
  Target t;
  last_status = 0;
  if (!crack(url, &t)) {
    say("\nURL inválida: %s\n", url);
    return NULL;
  }
  return request(t.secure ? "el servidor" : "el servidor (http)", &t, L"GET", api_key && *api_key ? api_key : NULL,
                 NULL, NULL, NULL, max, len);
}

char *http_post_json(const char *url, const char *body, int quiet_mode, size_t max) {
  Target t;
  char *out;
  last_status = last_error = 0;
  if (!crack(url, &t)) return NULL;
  quiet = quiet_mode;
  out = request("el servidor de reportes", &t, L"POST", NULL, NULL, NULL, body, max, NULL);
  quiet = 0;
  return out;
}
