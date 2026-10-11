#include "ini.h"

#include <ctype.h>
#include <string.h>

typedef struct { const char *p; size_t n; } Span;

static Span trim(const char *p, size_t n) {
  Span s;
  while (n && isspace((unsigned char)*p)) p++, n--;
  while (n && isspace((unsigned char)p[n - 1])) n--;
  s.p = p;
  s.n = n;
  return s;
}

static int same(Span s, const char *name) {
  size_t i;
  if (s.n != strlen(name)) return 0;
  for (i = 0; i < s.n; i++)
    if (tolower((unsigned char)s.p[i]) != tolower((unsigned char)name[i])) return 0;
  return 1;
}

/* One line: its span without the break, and where the next one starts. */
static Span next_line(const char **p) {
  Span s;
  s.p = *p;
  s.n = strcspn(*p, "\n");
  *p += s.n + (s.p[s.n] == '\n');
  if (s.n && s.p[s.n - 1] == '\r') s.n--;
  return s;
}

/* For a "[name]" line, its trimmed name; n = (size_t)-1 if the line is not a header. */
static Span header(Span line) {
  Span t = trim(line.p, line.n), none = {NULL, (size_t)-1};
  if (t.n < 2 || t.p[0] != '[' || t.p[t.n - 1] != ']') return none;
  return trim(t.p + 1, t.n - 2);
}

/* For a "key = value" line, the trimmed key and value; 0 for comments and other lines. */
static int pair(Span line, Span *k, Span *v) {
  Span t = trim(line.p, line.n);
  const char *eq;
  if (!t.n || t.p[0] == ';' || t.p[0] == '#') return 0;
  eq = memchr(t.p, '=', t.n);
  if (!eq) return 0;
  *k = trim(t.p, (size_t)(eq - t.p));
  *v = trim(eq + 1, t.n - (size_t)(eq - t.p) - 1);
  return k->n > 0;
}

int ini_get(const char *text, const char *section, const char *key, char *out, size_t cap) {
  const char *p = text ? text : "";
  int in = 0;
  out[0] = 0;
  while (*p) {
    Span line = next_line(&p), h = header(line), k, v;
    if (h.n != (size_t)-1) in = same(h, section);
    else if (in && pair(line, &k, &v) && same(k, key)) {
      if (v.n >= cap) return 0;
      memcpy(out, v.p, v.n);
      out[v.n] = 0;
      return 1;
    }
  }
  return 0;
}

static void add_pair(Buf *out, const char *key, const char *value) {
  buf_str(out, key);
  buf_str(out, " = ");
  buf_str(out, value);
  buf_str(out, "\n");
}

int ini_set(const char *text, const char *section, const char *key, const char *value, Buf *out) {
  const char *p = text ? text : "";
  int in = 0, found = 0, done = 0;
  if (strpbrk(section, "\r\n]") || strpbrk(key, "\r\n=") || strpbrk(value, "\r\n") || !*key) return 0;
  while (*p) {
    Span line = next_line(&p), h = header(line), k, v;
    if (h.n != (size_t)-1) {
      if (in && !done) { /* leaving our section without the key: add it here */
        add_pair(out, key, value);
        done = 1;
      }
      in = same(h, section);
      found |= in;
    } else if (in && pair(line, &k, &v) && same(k, key)) {
      if (!done) add_pair(out, key, value); /* replaced; a duplicate later is dropped */
      done = 1;
      continue;
    }
    buf_add(out, line.p, line.n);
    buf_str(out, "\n");
  }
  if (!done) {
    if (!found) {
      if (out->len) buf_str(out, "\n");
      buf_str(out, "[");
      buf_str(out, section);
      buf_str(out, "]\n");
    }
    add_pair(out, key, value);
  }
  return 1;
}
