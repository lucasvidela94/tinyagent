#include "url.h"

#include <stdlib.h>
#include <string.h>

int url_parse(const char *s, Url *u) {
  const char *host, *end, *p;
  size_t hn;
  memset(u, 0, sizeof *u);
  if (!strncmp(s, "https://", 8)) u->https = 1, host = s + 8;
  else if (!strncmp(s, "http://", 7)) host = s + 7;
  else return 0;
  for (p = s; *p; p++)
    if ((unsigned char)*p <= 0x20 || *p == 0x7F || *p == '#') return 0;
  end = host + strcspn(host, "/?");
  if (memchr(host, '@', (size_t)(end - host))) return 0; /* credentials in a URL: never */
  u->port = u->https ? 443 : 80;
  if (*host == '[') { /* IPv6 literal */
    const char *close = memchr(host, ']', (size_t)(end - host));
    if (!close) return 0;
    hn = (size_t)(close - host - 1);
    if (!hn || hn >= sizeof u->host) return 0;
    memcpy(u->host, host + 1, hn);
    p = close + 1;
  } else {
    const char *colon = memchr(host, ':', (size_t)(end - host));
    hn = (size_t)((colon ? colon : end) - host);
    if (!hn || hn >= sizeof u->host) return 0;
    memcpy(u->host, host, hn);
    p = colon ? colon : end;
  }
  if (*p == ':') {
    char *stop;
    unsigned long port = strtoul(p + 1, &stop, 10);
    if (stop == p + 1 || stop != end || port == 0 || port > 65535) return 0;
    u->port = (unsigned)port;
  } else if (p != end) {
    return 0;
  }
  if (strlen(end) + 2 > sizeof u->path) return 0;
  if (*end != '/') u->path[0] = '/';
  strcat(u->path, end); /* length checked above */
  return 1;
}
