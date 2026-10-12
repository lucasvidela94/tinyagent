/* Regression tests for src/core/httpwire.c: the HTTP/1.1 request and the incremental
 * response parser of the POSIX adapter. Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "httpwire.h"

static Buf got;
static void collect(void *ctx, const char *d, size_t n) {
  (void)ctx;
  buf_add(&got, d, n);
}

/* Feeds the whole response in pieces of `step` bytes; returns the final state. */
static int parse(const char *resp, size_t step, HttpwireResp *r) {
  size_t i, n = strlen(resp);
  int st = HTTPWIRE_MORE;
  free(got.p);
  memset(&got, 0, sizeof got);
  httpwire_init(r);
  for (i = 0; i < n && st == HTTPWIRE_MORE; i += step)
    st = httpwire_feed(r, resp + i, n - i < step ? n - i : step, collect, NULL);
  return st;
}

static int body_is(const char *want) { return !strcmp(got.p ? got.p : "", want); }

static void test_request(void) {
  Buf b = {0};
  Url u;
  url_parse("https://opencode.ai/zen/go/v1/chat/completions", &u);
  httpwire_request(&b, "POST", &u, "sk-x", "x-opencode-session", "tinyshell-1", "{}");
  CHECK(!strncmp(b.p, "POST /zen/go/v1/chat/completions HTTP/1.1\r\nHost: opencode.ai\r\n", 61));
  CHECK(strstr(b.p, "Authorization: Bearer sk-x\r\n") && strstr(b.p, "x-opencode-session: tinyshell-1\r\n"));
  CHECK(strstr(b.p, "Content-Length: 2\r\n") && !strcmp(b.p + b.len - 6, "\r\n\r\n{}"));
  free(b.p);
  memset(&b, 0, sizeof b);
  url_parse("http://[fe80::1]:8080/v1/models", &u);
  httpwire_request(&b, "GET", &u, "", "", "", NULL);
  CHECK(strstr(b.p, "Host: [fe80::1]:8080\r\n") && !strstr(b.p, "Authorization") && !strstr(b.p, "Content-"));
  free(b.p);
}

static void test_responses(void) {
  HttpwireResp r;
  const char *chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: text/event-stream\r\n\r\n"
                        "5;ext=1\r\ndata:\r\nb\r\n {\"a\":\"\xc3\xb1\"}\r\n0\r\nX-Trailer: 1\r\n\r\n";
  size_t step;
  for (step = 1; step <= 9; step += 4) { /* split anywhere: one byte at a time and more */
    CHECK(parse(chunked, step, &r) == HTTPWIRE_DONE && r.status == 200 && body_is("data: {\"a\":\"\xc3\xb1\"}"));
    httpwire_free(&r);
  }
  CHECK(parse("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nholaEXTRA", 3, &r) == HTTPWIRE_DONE && body_is("hola"));
  httpwire_free(&r);
  CHECK(parse("HTTP/1.0 200 OK\r\n\r\nuntil close", 5, &r) == HTTPWIRE_MORE && body_is("until close"));
  CHECK(httpwire_eof(&r) == HTTPWIRE_DONE); /* no length: the close ends it */
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nhola", 4, &r) == HTTPWIRE_MORE);
  CHECK(httpwire_eof(&r) == HTTPWIRE_BAD); /* cut short */
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 302 Found\r\nlocation:  https://objects.example/x?sig=1  \r\nContent-Length: 0\r\n\r\n", 7,
              &r) == HTTPWIRE_DONE &&
        r.status == 302 && !strcmp(r.location, "https://objects.example/x?sig=1"));
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 401 Unauthorized\r\nContent-Length: 2\r\n\r\n{}", 1, &r) ==
            HTTPWIRE_DONE &&
        r.status == 401 && body_is("{}"));
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 204 No Content\r\n\r\n", 2, &r) == HTTPWIRE_DONE && body_is(""));
  httpwire_free(&r);
}

static void test_malformed(void) {
  HttpwireResp r;
  char *big = (char *)malloc(40000);
  CHECK(parse("<html>502 Bad Gateway</html>\r\n\r\n", 4, &r) == HTTPWIRE_BAD);
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 200 OK\r\nContent-Length: -5\r\n\r\n", 4, &r) == HTTPWIRE_BAD);
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\n", 4, &r) == HTTPWIRE_BAD);
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n\r\n", 1, &r) == HTTPWIRE_BAD); /* empty size */
  httpwire_free(&r);
  CHECK(parse("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nabXY\r\n", 1, &r) == HTTPWIRE_BAD);
  httpwire_free(&r);
  memcpy(big, "HTTP/1.1 200 OK\r\nX: ", 20);
  memset(big + 20, 'a', 39000);
  big[39020] = 0;
  CHECK(parse(big, 4096, &r) == HTTPWIRE_BAD); /* head over 32 KB */
  httpwire_free(&r);
  free(big);
  CHECK(!strcmp(http_status_hint(429), " (usage limit; wait and try again)") && !strcmp(http_status_hint(200), ""));
}

int main(void) {
  test_request();
  test_responses();
  test_malformed();
  free(got.p);
  return CHECK_DONE("test_httpwire");
}
