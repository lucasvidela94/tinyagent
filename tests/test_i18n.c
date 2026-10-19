/* Regression tests for src/core/i18n.c: language detection and lookup (the table itself is
 * checked against the code by build/checks/i18n.sh). Run: ./build/test-core.sh */
#include <string.h>

#include "check.h"
#include "i18n.h"

static void test_parse(void) {
  int ok;
  CHECK(i18n_parse("es", &ok) == LANG_ES && ok);
  CHECK(i18n_parse("es_AR.UTF-8", &ok) == LANG_ES && ok);
  CHECK(i18n_parse("es_ES@euro", &ok) == LANG_ES && ok);
  CHECK(i18n_parse("ES", &ok) == LANG_ES && ok);
  CHECK(i18n_parse("en_US.UTF-8", &ok) == LANG_EN && ok);
  CHECK(i18n_parse("pt_BR.UTF-8", &ok) == LANG_EN && ok); /* no Portuguese yet: English */
  CHECK(i18n_parse("C", &ok) == LANG_EN && ok);
  CHECK(i18n_parse("est_EE", &ok) == LANG_EN); /* Estonian is not Spanish */
  CHECK(i18n_parse("auto", &ok) == LANG_EN && !ok); /* no preference */
  CHECK(i18n_parse("", &ok) == LANG_EN && !ok);
  CHECK(i18n_parse(NULL, &ok) == LANG_EN && !ok);
}

static void test_lookup(void) {
  i18n_set(LANG_EN);
  CHECK(!strcmp(tr("thinking..."), "thinking...")); /* English is the source */
  i18n_set(LANG_ES);
  CHECK(i18n_lang() == LANG_ES);
  CHECK(!strcmp(tr("thinking..."), "pensando..."));
  CHECK(!strcmp(tr("Yes, once"), "S\xc3\xad, una vez"));
  CHECK(!strcmp(tr("no such string"), "no such string")); /* missing: falls back to English */
  i18n_set(7); /* anything else is English */
  CHECK(i18n_lang() == LANG_EN);
}

int main(void) {
  test_parse();
  test_lookup();
  return CHECK_DONE("test_i18n");
}
