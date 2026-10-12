/* http(s) URLs, split the way an HTTP client needs them. Portable C (Windows uses
 * WinHttpCrackUrl; the POSIX HTTP adapter uses this). */
#ifndef TINYSHELL_URL_H
#define TINYSHELL_URL_H

typedef struct {
  int https;
  char host[256];  /* without brackets for an IPv6 literal */
  unsigned port;   /* 443 or 80 unless given */
  char path[2048]; /* "/" at least; the query string stays part of it */
} Url;

/* 1 if s is http://host[:port][/path][?query] or the https form; user:pass@, fragments,
 * spaces and control characters are refused. */
int url_parse(const char *s, Url *u);

#endif
