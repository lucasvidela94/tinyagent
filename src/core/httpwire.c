#include "httpwire.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define MAX_HEAD (32 * 1024)
#define MAX_SIZE_LINE 256

enum { S_HEAD, S_BODY, S_SIZE, S_DATA, S_DATA_END, S_TRAILER, S_DONE };

void httpwire_request(Buf *out, const char *method, const Url *u, const char *api_key, const char *session_header,
                      const char *session_id, const char *body) {
  char line[96];
  int v6 = strchr(u->host, ':') != NULL;
  buf_str(out, method);
  buf_str(out, " ");
  buf_str(out, u->path);
  buf_str(out, " HTTP/1.1\r\nHost: ");
  buf_str(out, v6 ? "[" : "");
  buf_str(out, u->host);
  buf_str(out, v6 ? "]" : "");
  if (u->port != (u->https ? 443u : 80u)) {
    snprintf(line, sizeof line, ":%u", u->port);
    buf_str(out, line);
  }
  buf_str(out, "\r\nUser-Agent: TinyShell/" TINYSHELL_VERSION "\r\nAccept: */*\r\nConnection: close\r\n");
  if (api_key && *api_key) { /* local servers often need no key */
    buf_str(out, "Authorization: Bearer ");
    buf_str(out, api_key);
    buf_str(out, "\r\n");
  }
  if (session_header && *session_header && session_id && *session_id) { /* e.g. OpenCode Go routing */
    buf_str(out, session_header);
    buf_str(out, ": ");
    buf_str(out, session_id);
    buf_str(out, "\r\n");
  }
  if (body) {
    snprintf(line, sizeof line, "Content-Type: application/json\r\nContent-Length: %lu\r\n",
             (unsigned long)strlen(body));
    buf_str(out, line);
  }
  buf_str(out, "\r\n");
  if (body) buf_str(out, body);
}

void httpwire_init(HttpwireResp *r) { memset(r, 0, sizeof *r); }
void httpwire_free(HttpwireResp *r) { free(r->line.p); }

static int name_is(const char *p, const char *name, size_t n) {
  size_t i;
  for (i = 0; i < n; i++)
    if (tolower((unsigned char)p[i]) != name[i]) return 0;
  return p[n] == ':';
}

/* The value of header `name` (lowercase) in the head, trimmed; "" if absent. */
static void header(const char *head, const char *name, char *out, size_t cap) {
  size_t n = strlen(name);
  const char *p = strstr(head, "\r\n");
  out[0] = 0;
  while (p && p[2]) {
    p += 2;
    if (name_is(p, name, n)) {
      const char *v = p + n + 1, *e = strstr(v, "\r\n");
      size_t vn;
      while (*v == ' ' || *v == '\t') v++;
      vn = (size_t)((e ? e : v + strlen(v)) - v);
      while (vn && (v[vn - 1] == ' ' || v[vn - 1] == '\t')) vn--;
      if (vn < cap) {
        memcpy(out, v, vn);
        out[vn] = 0;
      }
      return;
    }
    p = strstr(p, "\r\n");
  }
}

/* Status, framing and Location from the complete head. 0 if it is not an HTTP response. */
static int parse_head(HttpwireResp *r) {
  const char *h = r->line.p;
  char v[64];
  if (strncmp(h, "HTTP/1.", 7) || !isdigit((unsigned char)h[9]) || h[8] != ' ') return 0;
  r->status = atoi(h + 9);
  if (r->status < 100 || r->status > 999) return 0;
  header(h, "location", r->location, sizeof r->location);
  header(h, "transfer-encoding", v, sizeof v);
  r->chunked = strstr(v, "chunked") != NULL || strstr(v, "Chunked") != NULL;
  header(h, "content-length", v, sizeof v);
  if (r->chunked) r->state = S_SIZE;
  else if (*v) {
    char *end;
    r->left = strtoll(v, &end, 10);
    if (*end || r->left < 0) return 0;
    r->state = r->left ? S_BODY : S_DONE;
  } else {
    r->left = -1; /* until the server closes */
    r->state = S_BODY;
  }
  if (r->status == 204 || r->status == 304) r->state = S_DONE;
  r->line.len = 0;
  return 1;
}

/* One complete line in r->line for the chunk states. 0 if malformed. */
static int chunk_line(HttpwireResp *r) {
  char *end;
  if (r->state == S_DATA_END) {
    if (r->line.len) return 0; /* data must be followed by an empty line */
    r->state = S_SIZE;
    return 1;
  }
  if (r->state == S_TRAILER) {
    if (!r->line.len) r->state = S_DONE;
    return 1;
  }
  if (!r->line.len) return 0; /* a chunk size line cannot be empty */
  r->line.p[r->line.len] = 0;
  r->left = strtoll(r->line.p, &end, 16);
  if (end == r->line.p || r->left < 0 || (*end && *end != ';' && *end != ' ')) return 0;
  r->state = r->left ? S_DATA : S_TRAILER;
  return 1;
}

int httpwire_feed(HttpwireResp *r, const char *data, size_t n, HttpwireBody on_body, void *ctx) {
  size_t i = 0;
  while (i < n && r->state != S_DONE) {
    if (r->state == S_BODY || r->state == S_DATA) {
      size_t take = n - i;
      if (r->left >= 0 && (long long)take > r->left) take = (size_t)r->left;
      if (on_body && take) on_body(ctx, data + i, take);
      i += take;
      if (r->left >= 0 && (r->left -= (long long)take) == 0) r->state = r->state == S_BODY ? S_DONE : S_DATA_END;
      continue;
    }
    if (r->state == S_HEAD) {
      buf_add(&r->line, data + i++, 1);
      if (r->line.len > MAX_HEAD) return HTTPWIRE_BAD;
      if (r->line.len >= 4 && !memcmp(r->line.p + r->line.len - 4, "\r\n\r\n", 4)) {
        if (!parse_head(r)) return HTTPWIRE_BAD;
        if (r->status >= 100 && r->status < 200) { /* 100 Continue: another head follows */
          r->state = S_HEAD;
          r->line.len = 0;
        }
      }
      continue;
    }
    if (data[i] == '\n') { /* chunk size, end of chunk data, or trailer line */
      if (r->line.len && r->line.p[r->line.len - 1] == '\r') r->line.len--;
      if (!chunk_line(r)) return HTTPWIRE_BAD;
      r->line.len = 0;
    } else {
      buf_add(&r->line, data + i, 1);
      if (r->line.len > MAX_SIZE_LINE) return HTTPWIRE_BAD;
    }
    i++;
  }
  return r->state == S_DONE ? HTTPWIRE_DONE : HTTPWIRE_MORE;
}

int httpwire_eof(const HttpwireResp *r) {
  return r->state == S_DONE || (r->state == S_BODY && r->left < 0) ? HTTPWIRE_DONE : HTTPWIRE_BAD;
}

const char *http_status_hint(unsigned long status) {
  if (status == 401 || status == 403) return " (API key inválida o sin acceso al modelo)";
  if (status == 404) return " (no encontrado; ¿hay un release publicado?)";
  if (status == 429) return " (límite de uso; esperá y reintentá)";
  if (status >= 500) return " (problema del proveedor; reintentá más tarde)";
  return "";
}
