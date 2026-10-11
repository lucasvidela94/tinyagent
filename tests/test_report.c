/* Regression tests for src/report.c: error log ring, redaction of keys and identifying
 * data before reports leave the PC, and payload JSON. Run: ./build/test-core.sh */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/json.h"
#include "../src/report.h"
#include "check.h"

static void test_errlog(void) {
  Buf b = {0};
  int i;
  char d[16];
  CHECK(errlog_count() == 0);
  errlog_add("http", "401");
  errlog_dump(&b);
  CHECK(!strcmp(b.p, "http: 401\n"));
  free(b.p);
  for (i = 0; i < 30; i++) {
    snprintf(d, sizeof d, "%d", i);
    errlog_add("x", d);
  }
  CHECK(errlog_count() == 20);
  b.p = NULL; b.len = b.cap = 0;
  errlog_dump(&b);
  CHECK(!strncmp(b.p, "x: 10\n", 6)); /* oldest kept is the 11th of the 30 */
  CHECK(strstr(b.p, "x: 29\n") != NULL && strstr(b.p, "http") == NULL);
  free(b.p);
}

static void test_redact(void) {
  const char *names[] = {"C:\\Users\\Samira", "SAMIRA", "samira", ""};
  const char *labels[] = {"%USERPROFILE%", "<pc>", "<usuario>", "<vacio>"};
  char *r = redact("key sk-abcdefgh12345 y Authorization: Bearer abc.def-ghi_123 en C:\\users\\samira\\x "
                   "equipo SAMIRA sk-no",
                   names, labels, 4);
  CHECK(strstr(r, "abcdefgh12345") == NULL && strstr(r, "sk-<redactado>") != NULL);
  CHECK(strstr(r, "abc.def-ghi_123") == NULL && strstr(r, "Bearer <redactado>") != NULL);
  CHECK(strstr(r, "%USERPROFILE%\\x") != NULL);     /* home folder, case-insensitive */
  CHECK(strstr(r, "equipo <pc>") != NULL);
  CHECK(strstr(r, "sk-no") != NULL);                 /* too short to be a key */
  CHECK(strstr(r, "<vacio>") == NULL);               /* empty names never match */
  free(r);
}

static void test_payloads(void) {
  Json j = {0};
  char *s = report_json("id1", "0.6.0", "Windows 7", "no conecta", "publico \"x\"", "completo\nmulti");
  char *v;
  CHECK(json_parse(&j, s));
  v = json_str(&j, json_key(&j, 0, "full"));
  CHECK(!strcmp(v, "completo\nmulti"));
  free(v);
  json_free(&j);
  free(s);
  s = event_json("id1", "0.6.0", "Windows 7", "error", "{\"kind\":\"http\",\"code\":401}");
  CHECK(json_parse(&j, s));
  CHECK(json_type(&j, json_key(&j, 0, "data")) == '{');
  json_free(&j);
  free(s);
  s = event_json("id1", "0.6.0", "w", "session_start", NULL);
  CHECK(strstr(s, "\"data\":null") != NULL);
  free(s);
}

int main(void) {
  test_errlog();
  test_redact();
  test_payloads();
  return CHECK_DONE("test_report");
}
