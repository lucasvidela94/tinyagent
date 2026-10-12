#include "i18n.h"

#include <ctype.h>
#include <string.h>

static int lang = LANG_EN;

void i18n_set(int l) { lang = l == LANG_ES ? LANG_ES : LANG_EN; }
int i18n_lang(void) { return lang; }

int i18n_parse(const char *code, int *ok) {
  *ok = 0;
  if (!code || !*code || !strcmp(code, "auto")) return LANG_EN;
  *ok = 1;
  if (tolower((unsigned char)code[0]) == 'e' && tolower((unsigned char)code[1]) == 's' &&
      (!code[2] || code[2] == '_' || code[2] == '-' || code[2] == '.' || code[2] == '@'))
    return LANG_ES;
  if (!strncmp(code, "Spanish", 7) || !strncmp(code, "spanish", 7)) return LANG_ES;
  return LANG_EN;
}

const char *tr(const char *en) {
  int i;
  if (lang != LANG_ES) return en;
  for (i = 0; i < I18N_ES_COUNT; i++)
    if (!strcmp(I18N_ES[i].en, en)) return I18N_ES[i].es;
  return en;
}
