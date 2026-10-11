#include "charset.h"

#include <ctype.h>
#include <string.h>

static int contains_ci(const char *s, const char *word) {
  size_t i, n = strlen(word);
  for (; *s; s++) {
    for (i = 0; i < n && s[i] && tolower((unsigned char)s[i]) == word[i]; i++) {}
    if (i == n) return 1;
  }
  return 0;
}

int charset_from_locale(const char *locale) {
  if (!locale || !*locale) return CHARSET_ASCII;
  if (contains_ci(locale, "utf-8") || contains_ci(locale, "utf8")) return CHARSET_UTF8;
  if (contains_ci(locale, "8859") || contains_ci(locale, "latin") || contains_ci(locale, "@euro"))
    return CHARSET_LATIN1;
  if (!strcmp(locale, "C") || !strcmp(locale, "POSIX")) return CHARSET_ASCII;
  return CHARSET_LATIN1; /* "es_AR" without a codeset: glibc's default for it is ISO-8859-1 */
}

/* Base letters for U+00C0-U+00FF (x: multiplication sign, /: division sign). */
static const char LATIN_BASE[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";

static void ascii_for(unsigned long cp, Buf *out) {
  const char *fb = ascii_fallback(cp);
  char c[2] = {0, 0};
  if (fb) {
    buf_str(out, fb);
    return;
  }
  if (cp >= 0xC0 && cp <= 0xFF) c[0] = LATIN_BASE[cp - 0xC0];
  else if (cp == 0xA1) c[0] = '!';
  else if (cp == 0xBF) c[0] = '?';
  else if (cp == 0xAA) c[0] = 'a';
  else if (cp == 0xBA || cp == 0xB0) c[0] = 'o';
  else if (cp == 0xAB || cp == 0xBB) c[0] = '"';
  else if (cp == 0xB7) c[0] = '-'; /* the TUI's separator between footer fields */
  else c[0] = '?';
  buf_str(out, c);
}

void charset_encode(const char *utf8, int charset, Buf *out) {
  const unsigned char *p = (const unsigned char *)utf8;
  while (*p) {
    int n = utf8_len(p);
    unsigned long cp;
    if (*p < 0x80 || charset == CHARSET_UTF8) {
      buf_add(out, (const char *)p, n ? (size_t)n : 1);
      p += n ? n : 1;
      continue;
    }
    if (!n) { /* invalid byte */
      buf_str(out, "?");
      p++;
      continue;
    }
    cp = n == 2 ? ((p[0] & 0x1Ful) << 6) | (p[1] & 0x3F)
       : n == 3 ? ((p[0] & 0x0Ful) << 12) | ((p[1] & 0x3Ful) << 6) | (p[2] & 0x3F)
                : ((p[0] & 0x07ul) << 18) | ((p[1] & 0x3Ful) << 12) | ((p[2] & 0x3Ful) << 6) | (p[3] & 0x3F);
    p += n;
    if (charset == CHARSET_LATIN1 && cp <= 0xFF) {
      char c = (char)cp;
      buf_add(out, &c, 1);
    } else {
      ascii_for(cp, out);
    }
  }
}

void charset_decode(const char *text, size_t n, int charset, Buf *out) {
  size_t i;
  if (charset != CHARSET_LATIN1) {
    buf_add(out, text, n);
    return;
  }
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char)text[i];
    char two[2];
    if (c < 0x80) {
      buf_add(out, text + i, 1);
      continue;
    }
    two[0] = (char)(0xC0 | (c >> 6));
    two[1] = (char)(0x80 | (c & 0x3F));
    buf_add(out, two, 2);
  }
}
