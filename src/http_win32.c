#include "win32.h"

#include <winhttp.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

static const char *winhttp_hint(DWORD e) {
  switch (e) {
  case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "no se resolvió opencode.ai (DNS o sin red)";
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

static wchar_t *auth_headers(const char *api_key, size_t *wbytes) {
  Buf h = {0};
  char *s;
  wchar_t *w;
  int n;
  buf_str(&h, "Content-Type: application/json\r\nAuthorization: Bearer ");
  buf_str(&h, api_key);
  buf_str(&h, "\r\n");
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

/* Reads the body into b, refusing more than MAX_HTTP bytes. 0 on WinHTTP error or overflow. */
static int read_body(HINTERNET req, Buf *b) {
  char chunk[8192];
  DWORD got = 0;
  for (;;) {
    if (!WinHttpReadData(req, chunk, sizeof chunk, &got)) return 0;
    if (!got) return 1;
    if (b->len + got > MAX_HTTP) {
      say("\nLa respuesta supera %d bytes; se descartó.\n", MAX_HTTP);
      SetLastError(0);
      return 0;
    }
    buf_add(b, chunk, got);
  }
}

char *http_post(const char *body, const char *api_key) {
  HINTERNET ses = NULL, con = NULL, req = NULL;
  const char *stage = "abrir sesión";
  wchar_t *hdr = NULL;
  size_t hdr_bytes = 0;
  DWORD status = 0, sz = sizeof status, len = (DWORD)strlen(body), err;
  Buf b = {0};
  char *out = NULL;

  ses = WinHttpOpen(L"TinyShell/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);
  if (!ses) goto fail;
  enable_tls12(ses);
  WinHttpSetTimeouts(ses, 15000, 15000, 30000, 120000);
  stage = "conectar";
  if (!(con = WinHttpConnect(ses, API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0))) goto fail;
  if (!(req = WinHttpOpenRequest(con, L"POST", API_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE)))
    goto fail;
  hdr = auth_headers(api_key, &hdr_bytes);
  stage = "enviar";
  if (!WinHttpSendRequest(req, hdr, (DWORD)-1L, (LPVOID)body, len, len, 0)) goto fail;
  stage = "recibir";
  if (!WinHttpReceiveResponse(req, NULL)) goto fail;
  WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                      &status, &sz, WINHTTP_NO_HEADER_INDEX);
  if (!read_body(req, &b)) goto fail;
  if (status >= 200 && status < 300) {
    out = buf_take(&b);
  } else {
    if (b.len > 600) b.p[600] = 0;
    say("\nHTTP %lu%s: %s\n", (unsigned long)status, status_hint(status), b.p ? b.p : "");
  }
  goto done;

fail:
  err = GetLastError();
  if (err) say("\nNo se pudo contactar OpenCode Go (%s): error %lu, %s.\n", stage, (unsigned long)err, winhttp_hint(err));
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
