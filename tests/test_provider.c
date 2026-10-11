/* Regression tests for src/provider.c: defaults and validation of config.ini values
 * (URL scheme, header-injection attempts, names used as Credential Manager targets).
 * Run: ./build/test-core.sh */
#include <string.h>

#include "../src/provider.h"
#include "check.h"

static void test_defaults(void) {
  Provider p;
  provider_defaults(&p);
  CHECK(!strcmp(p.name, "opencode-go"));
  CHECK(!strcmp(p.model, "deepseek-v4.1-flash"));
  CHECK(!strcmp(p.session_header, "x-opencode-session"));
  CHECK(!provider_insecure(&p));
}

static void test_set(void) {
  Provider p;
  provider_defaults(&p);
  CHECK(provider_set(&p, "url", "https://api.openai.com/v1/chat/completions") == NULL);
  CHECK(provider_set(&p, "url", "http://192.168.0.10:1234/v1/chat/completions") == NULL);
  CHECK(provider_insecure(&p));
  CHECK(provider_set(&p, "url", "ftp://x/y") != NULL);
  CHECK(provider_set(&p, "url", "https://x/a b") != NULL);
  CHECK(!strcmp(p.url, "http://192.168.0.10:1234/v1/chat/completions")); /* unchanged on error */
  CHECK(provider_set(&p, "model", "deepseek/deepseek-chat") == NULL);
  CHECK(provider_set(&p, "model", "") != NULL);
  CHECK(provider_set(&p, "session_header", "") == NULL); /* none */
  CHECK(!strcmp(p.session_header, ""));
  CHECK(provider_set(&p, "session_header", "x-a\r\nAuthorization: Bearer x") != NULL); /* injection */
  CHECK(provider_set(&p, "session_header", "x a") != NULL);
  CHECK(provider_set(&p, "name", "openrouter") == NULL);
  CHECK(provider_set(&p, "name", "../x") != NULL);
  CHECK(provider_set(&p, "name", "") != NULL);
  CHECK(provider_set(&p, "nombre", "x") != NULL); /* unknown key */
}

static void test_lengths(void) {
  Provider p;
  char big[700];
  memset(big, 'a', sizeof big - 1);
  big[sizeof big - 1] = 0;
  provider_defaults(&p);
  CHECK(provider_set(&p, "model", big) != NULL);
  memcpy(big, "https://", 8);
  CHECK(provider_set(&p, "url", big) != NULL);
  CHECK(!strcmp(p.model, "deepseek-v4.1-flash"));
}

int main(void) {
  test_defaults();
  test_set();
  test_lengths();
  return CHECK_DONE("test_provider");
}
