/* Regression tests for src/core/provider.c: defaults and validation of config.ini values
 * (URL scheme, header-injection attempts, names used as Credential Manager targets).
 * Run: ./build/test-core.sh */
#include <string.h>

#include "json.h"
#include "provider.h"
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

static void test_models(void) {
  Provider p;
  char url[600], ids[4][128];
  provider_defaults(&p);
  CHECK(provider_models_url(&p, url, sizeof url) && !strcmp(url, "https://opencode.ai/zen/go/v1/models"));
  provider_set(&p, "url", "http://10.0.0.5:1234/v1/chat/completions?api-version=1");
  CHECK(provider_models_url(&p, url, sizeof url) && !strcmp(url, "http://10.0.0.5:1234/v1/models?api-version=1"));
  provider_set(&p, "url", "https://example.com/api/generate");
  CHECK(!provider_models_url(&p, url, sizeof url)); /* other shapes: no list, type the name */
  CHECK(provider_parse_models("{\"object\":\"list\",\"data\":[{\"id\":\"glm-5.1\"},{\"id\":\"kimi-k3\"},"
                              "{\"id\":\"con espacio\"},{\"x\":1},{\"id\":\"qwen3.8-max\"},{\"id\":\"extra\"}]}",
                              ids, 3) == 3);
  CHECK(!strcmp(ids[0], "glm-5.1") && !strcmp(ids[1], "kimi-k3") && !strcmp(ids[2], "qwen3.8-max"));
  CHECK(provider_parse_models("<html>", ids, 4) == 0);
  CHECK(provider_parse_models("{\"data\":[]}", ids, 4) == 0);
  CHECK(provider_preset_count() >= 1);
  CHECK(strstr(provider_preset_label(0), "OpenCode Go") != NULL);
}

int main(void) {
  test_defaults();
  test_set();
  test_lengths();
  test_models();
  return CHECK_DONE("test_provider");
}
