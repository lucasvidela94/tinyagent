#include "credfile.h"

#include <string.h>

/* Length of the line at p without its line break; *next is set past the break. */
static size_t line_at(const char *p, const char **next) {
  size_t n = strcspn(p, "\r\n");
  *next = p + n;
  if (**next == '\r') (*next)++;
  if (**next == '\n') (*next)++;
  return n;
}

/* 1 if the line (n bytes) belongs to provider: "<provider>\t...". */
static int is_for(const char *line, size_t n, const char *provider) {
  size_t pn = strlen(provider);
  return n > pn && !strncmp(line, provider, pn) && line[pn] == '\t';
}

int credfile_get(const char *text, const char *provider, char *out, size_t cap) {
  const char *p = text ? text : "", *next;
  size_t pn = strlen(provider);
  out[0] = 0;
  while (*p) {
    size_t n = line_at(p, &next);
    if (is_for(p, n, provider)) {
      size_t kn = n - pn - 1;
      if (kn == 0 || kn >= cap) return 0;
      memcpy(out, p + pn + 1, kn);
      out[kn] = 0;
      return 1;
    }
    p = next;
  }
  return 0;
}

static int clean(const char *s) { return s && !strpbrk(s, "\t\r\n"); }

int credfile_set(const char *text, const char *provider, const char *key, Buf *out) {
  const char *p = text ? text : "", *next;
  if (!*provider || !clean(provider) || (key && (!*key || !clean(key)))) return 0;
  while (*p) {
    size_t n = line_at(p, &next);
    if (n && !is_for(p, n, provider)) {
      buf_add(out, p, n);
      buf_str(out, "\n");
    }
    p = next;
  }
  if (key) {
    buf_str(out, provider);
    buf_str(out, "\t");
    buf_str(out, key);
    buf_str(out, "\n");
  }
  return 1;
}
