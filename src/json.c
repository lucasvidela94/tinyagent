#include "json.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

static int push(Json *j, int start, int end, const int *stack, int sp) {
  Tok *parent = sp ? &j->tok[stack[sp - 1]] : NULL;
  if (j->n >= MAX_TOKENS) return -1;
  /* inside an object, even positions are keys and must be strings */
  if (parent && j->src[parent->start] == '{' && parent->size % 2 == 0 && j->src[start] != '"') return -1;
  if (parent) parent->size++;
  if (j->n == j->cap) {
    j->cap = j->cap ? j->cap * 2 : 64;
    if (j->cap > MAX_TOKENS) j->cap = MAX_TOKENS;
    j->tok = (Tok *)xrealloc(j->tok, (size_t)j->cap * sizeof(Tok));
  }
  j->tok[j->n].start = start;
  j->tok[j->n].end = end;
  j->tok[j->n].size = 0;
  return j->n++;
}

int json_parse(Json *j, const char *s) {
  int stack[MAX_DEPTH], sp = 0, i = 0;
  j->src = s;
  j->n = 0;
  for (;;) {
    unsigned char c;
    while (s[i] && (isspace((unsigned char)s[i]) || s[i] == ',' || s[i] == ':')) i++;
    c = (unsigned char)s[i];
    if (!c) break;
    if (sp == 0 && j->n > 0) return 0; /* data after the top-level value */
    if (c == '{' || c == '[') {
      int t;
      if (sp == MAX_DEPTH || (t = push(j, i, -1, stack, sp)) < 0) return 0;
      stack[sp++] = t;
      i++;
    } else if (c == '}' || c == ']') {
      Tok *t;
      if (!sp) return 0;
      t = &j->tok[stack[sp - 1]];
      if ((c == '}') != (j->src[t->start] == '{')) return 0;
      if (c == '}' && t->size % 2) return 0; /* key without value */
      t->end = ++i;
      sp--;
    } else if (c == '"') {
      int st = i++;
      while (s[i] && s[i] != '"') i += (s[i] == '\\' && s[i + 1]) ? 2 : 1;
      if (!s[i]) return 0;
      if (push(j, st, ++i, stack, sp) < 0) return 0;
    } else {
      int st = i;
      if (!strchr("-0123456789tfn", c)) return 0;
      while (s[i] && !isspace((unsigned char)s[i]) && !strchr(",:[]{}\"", s[i])) i++;
      if (push(j, st, i, stack, sp) < 0) return 0;
    }
  }
  return sp == 0 && j->n > 0;
}

void json_free(Json *j) {
  free(j->tok);
  j->tok = NULL;
  j->n = j->cap = 0;
}

char json_type(const Json *j, int i) { return (i >= 0 && i < j->n) ? j->src[j->tok[i].start] : 0; }

int json_size(const Json *j, int i) {
  char t = json_type(j, i);
  return (t == '{' || t == '[') ? j->tok[i].size : 0;
}

static int next(const Json *j, int i) { /* first token after i and its descendants */
  int e = j->tok[i].end;
  for (i++; i < j->n && j->tok[i].start < e; i++) {}
  return i;
}

int json_key(const Json *j, int obj, const char *name) {
  int end, k;
  size_t len = strlen(name);
  if (json_type(j, obj) != '{') return -1;
  end = next(j, obj);
  for (k = obj + 1; k < end; k = next(j, k + 1)) {
    const Tok *t = &j->tok[k];
    if ((size_t)(t->end - t->start - 2) == len && !strncmp(j->src + t->start + 1, name, len)) return k + 1;
  }
  return -1;
}

int json_index(const Json *j, int arr, int idx) {
  int k;
  if (json_type(j, arr) != '[' || idx < 0 || idx >= j->tok[arr].size) return -1;
  for (k = arr + 1; idx > 0; idx--) k = next(j, k);
  return k;
}

char *json_raw(const Json *j, int i) {
  Buf b = {0};
  if (!json_type(j, i)) return xstrdup("null");
  buf_add(&b, j->src + j->tok[i].start, (size_t)(j->tok[i].end - j->tok[i].start));
  return buf_take(&b);
}

static void put_utf8(Buf *b, unsigned long cp) {
  char o[4];
  size_t n;
  if (cp < 0x80) { o[0] = (char)cp; n = 1; }
  else if (cp < 0x800) { o[0] = (char)(0xC0 | cp >> 6); o[1] = (char)(0x80 | (cp & 0x3F)); n = 2; }
  else if (cp < 0x10000) {
    o[0] = (char)(0xE0 | cp >> 12); o[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); o[2] = (char)(0x80 | (cp & 0x3F)); n = 3;
  } else {
    o[0] = (char)(0xF0 | cp >> 18); o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); o[3] = (char)(0x80 | (cp & 0x3F)); n = 4;
  }
  buf_add(b, o, n);
}

static long hex4(const char *p, const char *end) {
  long v = 0;
  int k;
  if (end - p < 4) return -1;
  for (k = 0; k < 4; k++) {
    char c = p[k];
    v <<= 4;
    if (c >= '0' && c <= '9') v |= c - '0';
    else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
    else return -1;
  }
  return v;
}

char *json_str(const Json *j, int i) {
  Buf b = {0};
  const char *p, *end;
  char t = json_type(j, i);
  if (!t || (t == 'n' && j->tok[i].end - j->tok[i].start == 4)) return xstrdup("");
  if (t != '"') return json_raw(j, i);
  p = j->src + j->tok[i].start + 1;
  end = j->src + j->tok[i].end - 1;
  while (p < end) {
    long cp, lo;
    if (*p != '\\' || p + 1 >= end) { buf_add(&b, p++, 1); continue; }
    p++;
    switch (*p++) {
    case 'n': buf_str(&b, "\n"); break;
    case 'r': buf_str(&b, "\r"); break;
    case 't': buf_str(&b, "\t"); break;
    case 'b': buf_str(&b, "\b"); break;
    case 'f': buf_str(&b, "\f"); break;
    case 'u':
      if ((cp = hex4(p, end)) < 0) { put_utf8(&b, 0xFFFD); break; }
      p += 4;
      if (cp >= 0xD800 && cp <= 0xDBFF && end - p >= 6 && p[0] == '\\' && p[1] == 'u' &&
          (lo = hex4(p + 2, end)) >= 0xDC00 && lo <= 0xDFFF) {
        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
        p += 6;
      }
      if (cp == 0 || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD; /* NUL would truncate */
      put_utf8(&b, (unsigned long)cp);
      break;
    default: buf_add(&b, p - 1, 1); break; /* \" \\ \/ and unknown escapes */
    }
  }
  return buf_take(&b);
}
