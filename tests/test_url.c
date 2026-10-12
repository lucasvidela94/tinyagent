/* Regression tests for src/core/url.c. Run: ./build/test-core.sh */
#include <string.h>

#include "check.h"
#include "url.h"

int main(void) {
  Url u;
  CHECK(url_parse("https://opencode.ai/zen/go/v1/chat/completions", &u) && u.https && u.port == 443 &&
        !strcmp(u.host, "opencode.ai") && !strcmp(u.path, "/zen/go/v1/chat/completions"));
  CHECK(url_parse("http://192.168.0.10:1234/v1/chat/completions", &u) && !u.https && u.port == 1234 &&
        !strcmp(u.host, "192.168.0.10"));
  CHECK(url_parse("https://x.io", &u) && !strcmp(u.path, "/"));
  CHECK(url_parse("https://x.io?a=1&b=2", &u) && !strcmp(u.path, "/?a=1&b=2")); /* query kept */
  CHECK(url_parse("https://x.io/d?X-Amz=1/2", &u) && !strcmp(u.host, "x.io") && !strcmp(u.path, "/d?X-Amz=1/2"));
  CHECK(url_parse("http://[fe80::1]:8080/v1", &u) && !strcmp(u.host, "fe80::1") && u.port == 8080);
  CHECK(!url_parse("ftp://x.io/", &u));
  CHECK(!url_parse("https://user:pw@x.io/", &u)); /* credentials refused */
  CHECK(!url_parse("https://x.io:0/", &u) && !url_parse("https://x.io:70000/", &u) && !url_parse("https://x.io:8a/", &u));
  CHECK(!url_parse("https:///path", &u) && !url_parse("https://", &u));
  CHECK(!url_parse("https://x.io/a b", &u) && !url_parse("https://x.io/\r\nHost: evil", &u)); /* no header injection */
  CHECK(!url_parse("https://x.io/#frag", &u));
  CHECK(!url_parse("http://[::1/", &u));
  return CHECK_DONE("test_url");
}
