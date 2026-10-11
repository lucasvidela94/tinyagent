/* Regression tests for src/core/json.c and src/core/util.c: malformed and oversized input,
 * missing fields, null content, \u escapes, invalid UTF-8, text fitting for the TUI. Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "conversation.h"
#include "json.h"
#include "util.h"
#include "check.h"

static int parses(const char *s) {
  Json j = {0};
  int ok = json_parse(&j, s);
  json_free(&j);
  return ok;
}

static void test_json_structure(void) {
  char deep[MAX_DEPTH * 2 + 8];
  int i;
  CHECK(parses("{}"));
  CHECK(parses("[1,2,{\"a\":[true,false,null]}]"));
  CHECK(parses("\"solo\""));
  CHECK(!parses(""));
  CHECK(!parses("   "));
  CHECK(!parses("{"));
  CHECK(!parses("{\"a\":1"));
  CHECK(!parses("{\"a\":\"sin cerrar}"));
  CHECK(!parses("{]"));
  CHECK(!parses("[}"));
  CHECK(!parses("}"));
  CHECK(!parses("{\"a\"}"));     /* key without value */
  CHECK(!parses("{1:2}"));       /* non-string key */
  CHECK(!parses("{} {}"));       /* two top-level values */
  CHECK(!parses("{\"a\":xyz}")); /* garbage primitive */
  CHECK(!parses("<html>502 Bad Gateway</html>"));
  for (i = 0; i < MAX_DEPTH + 1; i++) deep[i] = '[';
  for (i = 0; i < MAX_DEPTH + 1; i++) deep[MAX_DEPTH + 1 + i] = ']';
  deep[2 * (MAX_DEPTH + 1)] = 0;
  CHECK(!parses(deep)); /* one level too deep */
  deep[MAX_DEPTH] = 0;
  CHECK(!parses(deep)); /* unclosed */
}

/* A failed parse frees its own tokens, so callers that skip json_free on failure do not
 * leak (LeakSanitizer fails this test otherwise). Input found by build/fuzz.sh. */
static void test_json_failure_frees(void) {
  Json j = {0};
  CHECK(!json_parse(&j, "{\"choices\":[{\"message\":{\"content\":\"hol\x16\"}$]}"));
  CHECK(j.tok == NULL && j.n == 0);
}

static void test_json_too_many_tokens(void) {
  Buf b = {0};
  int i;
  buf_str(&b, "[");
  for (i = 0; i < MAX_TOKENS + 10; i++) buf_str(&b, "1,");
  buf_str(&b, "1]");
  CHECK(!parses(b.p));
  free(b.p);
}

static void test_json_lookup(void) {
  Json j = {0};
  char *s;
  CHECK(json_parse(&j, "{\"choices\":[],\"x\":{\"content\":null,\"n\":12,\"o\":{\"k\":1}}}"));
  CHECK(json_index(&j, json_key(&j, 0, "choices"), 0) < 0); /* empty array: no element */
  CHECK(json_key(&j, 0, "missing") < 0);
  CHECK(json_key(&j, json_key(&j, 0, "choices"), "a") < 0); /* key lookup on an array */
  CHECK(json_key(&j, -1, "a") < 0);
  CHECK(json_size(&j, -1) == 0);
  s = json_str(&j, json_key(&j, json_key(&j, 0, "x"), "content"));
  CHECK(!strcmp(s, "")); /* null content is empty, not "null" */
  free(s);
  s = json_str(&j, json_key(&j, json_key(&j, 0, "x"), "n"));
  CHECK(!strcmp(s, "12"));
  free(s);
  s = json_str(&j, json_key(&j, json_key(&j, 0, "x"), "o"));
  CHECK(!strcmp(s, "{\"k\":1}"));
  free(s);
  json_free(&j);
  /* a value spelled like a key must not match */
  CHECK(json_parse(&j, "{\"a\":\"b\",\"b\":\"c\"}"));
  s = json_str(&j, json_key(&j, 0, "b"));
  CHECK(!strcmp(s, "c"));
  free(s);
  json_free(&j);
}

static void test_json_unescape(void) {
  Json j = {0};
  char *s;
  CHECK(json_parse(&j, "[\"Configuraci\\u00f3n \\\"ok\\\" \\\\ \\/ \\n\", \"\\ud83d\\ude00\", \"\\ud83d x\", "
                       "\"\\u00zz\", \"\\u0000\"]"));
  s = json_str(&j, json_index(&j, 0, 0));
  CHECK(!strcmp(s, "Configuraci\xc3\xb3n \"ok\" \\ / \n"));
  free(s);
  s = json_str(&j, json_index(&j, 0, 1));
  CHECK(!strcmp(s, "\xf0\x9f\x98\x80")); /* surrogate pair */
  free(s);
  s = json_str(&j, json_index(&j, 0, 2));
  CHECK(!strcmp(s, "\xef\xbf\xbd x")); /* lone surrogate */
  free(s);
  s = json_str(&j, json_index(&j, 0, 3));
  CHECK(!strncmp(s, "\xef\xbf\xbd", 3)); /* bad hex */
  free(s);
  s = json_str(&j, json_index(&j, 0, 4));
  CHECK(!strcmp(s, "\xef\xbf\xbd")); /* NUL never truncates a string */
  free(s);
  json_free(&j);
}

static void test_jesc(void) {
  Json j = {0};
  char *e, *c;
  e = jesc("a\"b\\c\n\r\t\x01 \xc3\xa1");
  CHECK(!strcmp(e, "a\\\"b\\\\c\\n\\r\\t\\u0001 \xc3\xa1"));
  free(e);
  e = jesc("Configuraci\xa2n"); /* CP850 byte from a Spanish systeminfo: invalid UTF-8 */
  CHECK(!strcmp(e, "Configuraci\\ufffdn"));
  CHECK(utf8_valid(e));
  free(e);
  e = jesc("\xed\xa0\x80"); /* encoded surrogate is invalid UTF-8 */
  CHECK(!strcmp(e, "\\ufffd\\ufffd\\ufffd"));
  free(e);
  e = message_json("user", "\xc2\xbfqu\xc3\xa9 pasa? \"x\"\n"); /* round trip */
  CHECK(json_parse(&j, e));
  c = json_str(&j, json_key(&j, 0, "content"));
  CHECK(!strcmp(c, "\xc2\xbfqu\xc3\xa9 pasa? \"x\"\n"));
  free(c);
  json_free(&j);
  free(e);
}

static void test_utf8(void) {
  CHECK(utf8_valid("ascii"));
  CHECK(utf8_valid("\xc3\xb1 \xe2\x82\xac \xf0\x9f\x98\x80"));
  CHECK(!utf8_valid("\xa2"));
  CHECK(!utf8_valid("\xc3"));     /* truncated */
  CHECK(!utf8_valid("\xc0\xaf")); /* overlong */
  CHECK(!utf8_valid("\xf5\x80\x80\x80"));
}

static void test_text_fit(void) {
  CHECK(utf8_cols("hola", 4) == 4);
  CHECK(utf8_cols("versi\xc3\xb3n", 8) == 7); /* 8 bytes, 7 columns */
  CHECK(text_fit("hola mundo", 20) == 10);
  CHECK(text_fit("hola mundo", 7) == 4);       /* breaks at the space, not inside "mundo" */
  CHECK(text_fit("holamundo", 4) == 4);        /* no space: hard cut */
  CHECK(text_fit("ab\ncd", 10) == 2);          /* stops before a newline */
  CHECK(text_fit("\ncd", 10) == 0);
  CHECK(text_fit("", 10) == 0);
  CHECK(text_fit("\xc3\xb3\xc3\xb3\xc3\xb3", 2) == 4); /* never splits a code point */
  CHECK(text_fit("abc", 0) == 1);               /* always makes progress */
}

static void test_ascii_fallback(void) {
  CHECK(!strcmp(ascii_fallback(0x2192), "->"));   /* the arrow the model used in the first real test */
  CHECK(!strcmp(ascii_fallback(0x2026), "..."));
  CHECK(!strcmp(ascii_fallback(0x201C), "\""));
  CHECK(!strcmp(ascii_fallback(0x1F600), ""));     /* emoji are dropped */
  CHECK(!strcmp(ascii_fallback(0xFE0F), ""));      /* invisible variation selector */
  CHECK(ascii_fallback(0x00E1) == NULL);           /* accented letters exist in the code page */
  CHECK(ascii_fallback('a') == NULL);
}

int main(void) {
  test_json_structure();
  test_json_failure_frees();
  test_json_too_many_tokens();
  test_json_lookup();
  test_json_unescape();
  test_jesc();
  test_utf8();
  test_text_fit();
  test_ascii_fallback();
  return CHECK_DONE("test_json");
}
