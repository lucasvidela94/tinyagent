#include "version.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Parses "X.Y.Z" or "vX.Y.Z"; trailing text after Z (e.g. "\r\n") is ignored. */
static int parse(const char *s, long v[3]) {
  int i;
  char *end;
  if (*s == 'v' || *s == 'V') s++;
  for (i = 0; i < 3; i++) {
    if (!isdigit((unsigned char)*s)) return 0;
    v[i] = strtol(s, &end, 10);
    s = end;
    if (i < 2 && *s++ != '.') return 0;
  }
  return !isdigit((unsigned char)*s) && *s != '.';
}

int version_compare(const char *a, const char *b) {
  long x[3], y[3];
  int i;
  if (!parse(a, x) || !parse(b, y)) return VERSION_INVALID;
  for (i = 0; i < 3; i++)
    if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
  return 0;
}

int version_from_output(const char *output, char *out, size_t cap) {
  static const char prefix[] = "tinyshell ";
  long v[3];
  size_t n;
  if (strncmp(output, prefix, sizeof prefix - 1)) return 0;
  output += sizeof prefix - 1;
  if (!parse(output, v)) return 0;
  n = strspn(output, "0123456789.vV");
  if (n + 1 > cap) return 0;
  memcpy(out, output, n);
  out[n] = 0;
  return 1;
}
