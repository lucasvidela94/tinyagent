#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xrealloc(void *p, size_t n) {
  void *q = realloc(p, n ? n : 1);
  if (!q) {
    fprintf(stderr, "\nERROR: Memoria insuficiente\n");
    exit(2);
  }
  return q;
}

char *xstrdup(const char *s) {
  size_t n = strlen(s) + 1;
  return (char *)memcpy(xrealloc(NULL, n), s, n);
}

void buf_add(Buf *b, const char *s, size_t n) {
  if (b->len + n + 1 > b->cap) {
    size_t c = b->cap ? b->cap : 256;
    while (c < b->len + n + 1) c *= 2;
    b->p = (char *)xrealloc(b->p, c);
    b->cap = c;
  }
  memcpy(b->p + b->len, s, n);
  b->len += n;
  b->p[b->len] = 0;
}

void buf_str(Buf *b, const char *s) { buf_add(b, s, strlen(s)); }

char *buf_take(Buf *b) {
  char *p = b->p ? b->p : xstrdup("");
  b->p = NULL;
  b->len = b->cap = 0;
  return p;
}

void buf_add_capped(Buf *b, const char *s, size_t n, size_t limit, size_t *dropped) {
  size_t room = b->len < limit ? limit - b->len : 0;
  size_t take = n < room ? n : room;
  if (take) buf_add(b, s, take);
  *dropped += n - take;
}

void trim_line(char *s) {
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
}

/* Reads at most up to the first invalid byte, so a NUL terminator is never passed. */
int utf8_len(const unsigned char *s) {
  unsigned char c = s[0];
  if (c < 0x80) return 1;
  if (c >= 0xC2 && c <= 0xDF) return (s[1] & 0xC0) == 0x80 ? 2 : 0;
  if (c >= 0xE0 && c <= 0xEF) {
    if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
    if ((c == 0xE0 && s[1] < 0xA0) || (c == 0xED && s[1] > 0x9F)) return 0; /* overlong, surrogate */
    return 3;
  }
  if (c >= 0xF0 && c <= 0xF4) {
    if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80) return 0;
    if ((c == 0xF0 && s[1] < 0x90) || (c == 0xF4 && s[1] > 0x8F)) return 0; /* overlong, > U+10FFFF */
    return 4;
  }
  return 0;
}

int utf8_valid(const char *s) {
  const unsigned char *p = (const unsigned char *)s;
  while (*p) {
    int n = utf8_len(p);
    if (!n) return 0;
    p += n;
  }
  return 1;
}

int utf8_cols(const char *s, size_t n) {
  int cols = 0;
  size_t i;
  for (i = 0; i < n && s[i]; i++)
    if (((unsigned char)s[i] & 0xC0) != 0x80) cols++;
  return cols;
}

size_t text_fit(const char *s, int cols) {
  const unsigned char *p = (const unsigned char *)s;
  size_t n = 0, last_space = 0;
  int c = 0, len;
  while (p[n] && p[n] != '\n' && c < cols) {
    if (p[n] == ' ') last_space = n;
    len = utf8_len(p + n);
    n += len ? (size_t)len : 1;
    c++;
  }
  if (p[n] && p[n] != '\n' && p[n] != ' ' && last_space > 0) return last_space;
  if (n == 0 && p[0] && p[0] != '\n') return (len = utf8_len(p)) != 0 ? (size_t)len : 1;
  return n;
}

static const struct { unsigned short cp; const char *ascii; } FALLBACK[] = {
    {0x00A0, " "},   {0x2013, "-"},  {0x2014, "-"},  {0x2018, "'"},  {0x2019, "'"},   {0x201A, "'"},
    {0x201C, "\""}, {0x201D, "\""}, {0x201E, "\""}, {0x2022, "*"},  {0x2026, "..."}, {0x2039, "<"},
    {0x203A, ">"},   {0x2190, "<-"}, {0x2191, "^"},  {0x2192, "->"}, {0x2193, "v"},   {0x2194, "<->"},
    {0x21D2, "=>"},  {0x2212, "-"},  {0x2248, "~"},  {0x2260, "!="}, {0x2264, "<="},  {0x2265, ">="},
    {0x2705, "v"},   {0x2713, "v"},  {0x2714, "v"},  {0x2717, "x"},  {0x2718, "x"},   {0x274C, "x"},
    {0x26A0, "!"},   {0x2139, "i"},  {0x200B, ""},   {0x200C, ""},   {0x200D, ""},    {0xFE0F, ""},
};

const char *ascii_fallback(unsigned long cp) {
  size_t i;
  if (cp >= 0x10000) return ""; /* emoji and other astral symbols: drop rather than show "??" */
  for (i = 0; i < sizeof FALLBACK / sizeof FALLBACK[0]; i++)
    if (FALLBACK[i].cp == cp) return FALLBACK[i].ascii;
  return NULL;
}

char *jesc(const char *s) {
  Buf b = {0};
  const unsigned char *p = (const unsigned char *)s;
  while (*p) {
    unsigned char c = *p;
    char tmp[8];
    int n;
    if (c == '"' || c == '\\') {
      tmp[0] = '\\';
      tmp[1] = (char)c;
      buf_add(&b, tmp, 2);
      p++;
    } else if (c == '\n') { buf_str(&b, "\\n"); p++; }
    else if (c == '\r') { buf_str(&b, "\\r"); p++; }
    else if (c == '\t') { buf_str(&b, "\\t"); p++; }
    else if (c < 0x20) {
      snprintf(tmp, sizeof tmp, "\\u%04x", c);
      buf_add(&b, tmp, 6);
      p++;
    } else if ((n = utf8_len(p)) != 0) {
      buf_add(&b, (const char *)p, (size_t)n);
      p += n;
    } else {
      buf_str(&b, "\\ufffd");
      p++;
    }
  }
  return buf_take(&b);
}
