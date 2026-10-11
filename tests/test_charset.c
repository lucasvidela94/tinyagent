/* Regression tests for src/core/charset.c: text for non-UTF-8 terminals (LANG=C or a
 * Latin-1 locale on old Linux distros). Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "check.h"

static int encodes(const char *in, int cs, const char *want) {
  Buf b = {0};
  int ok;
  charset_encode(in, cs, &b);
  ok = !strcmp(b.p ? b.p : "", want);
  free(b.p);
  return ok;
}

static void test_locale(void) {
  CHECK(charset_from_locale("es_AR.UTF-8") == CHARSET_UTF8);
  CHECK(charset_from_locale("en_US.utf8") == CHARSET_UTF8);
  CHECK(charset_from_locale("es_ES@euro") == CHARSET_LATIN1);
  CHECK(charset_from_locale("es_ES.ISO-8859-15") == CHARSET_LATIN1);
  CHECK(charset_from_locale("es_AR") == CHARSET_LATIN1); /* glibc default codeset */
  CHECK(charset_from_locale("C") == CHARSET_ASCII);
  CHECK(charset_from_locale("POSIX") == CHARSET_ASCII);
  CHECK(charset_from_locale("") == CHARSET_ASCII);
  CHECK(charset_from_locale(NULL) == CHARSET_ASCII);
}

static void test_encode(void) {
  /* "¿Qué año? ─ ✓" */
  const char *s = "\xc2\xbfQu\xc3\xa9 a\xc3\xb1o? \xe2\x94\x80 \xe2\x9c\x93";
  CHECK(encodes(s, CHARSET_UTF8, s));
  CHECK(encodes(s, CHARSET_LATIN1, "\xbfQu\xe9 a\xf1o? - v"));
  CHECK(encodes(s, CHARSET_ASCII, "?Que ano? - v"));
  CHECK(encodes("\xc3\x91 \xc3\x9c \xc2\xa1", CHARSET_ASCII, "N U !"));
  CHECK(encodes("\xe4\xb8\xad", CHARSET_LATIN1, "?")); /* CJK: no stand-in */
  CHECK(encodes("\xf0\x9f\x98\x80x", CHARSET_ASCII, "x")); /* emoji dropped */
  CHECK(encodes("a\xffz", CHARSET_ASCII, "a?z"));          /* invalid byte */
  CHECK(encodes("a\xffz", CHARSET_LATIN1, "a?z"));
}

static void test_decode(void) {
  Buf b = {0};
  charset_decode("a\xf1o \xbf", 5, CHARSET_LATIN1, &b);
  CHECK(b.p && !strcmp(b.p, "a\xc3\xb1o \xc2\xbf"));
  free(b.p);
  b.p = NULL;
  b.len = b.cap = 0;
  charset_decode("plain", 5, CHARSET_ASCII, &b);
  CHECK(b.p && !strcmp(b.p, "plain"));
  free(b.p);
}

int main(void) {
  test_locale();
  test_encode();
  test_decode();
  return CHECK_DONE("test_charset");
}
