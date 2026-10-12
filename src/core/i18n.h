/* The interface language. Source strings are English; tr() returns the Spanish
 * translation (core/i18n_es.c) when the interface is Spanish. Every user-facing string goes
 * through tr(), including what the model also reads (command results, refusals): the
 * conversation is in the user's language anyway. build/checks/i18n.sh checks that each
 * tr("...") has a translation with the same printf conversions. Portable C. */
#ifndef TINYSHELL_I18N_H
#define TINYSHELL_I18N_H

enum { LANG_EN, LANG_ES };

void i18n_set(int lang);
int i18n_lang(void);

/* LANG_ES for "es", "es_AR.UTF-8", "Spanish"...; LANG_EN for anything else, NULL or "".
 * "auto" and the empty string mean "no preference": *ok is set to 0 then. */
int i18n_parse(const char *code, int *ok);

const char *tr(const char *en); /* never NULL; the English text if there is no translation */

/* Marks a string translated later, where it is shown (tables, return values): a no-op the
 * check script reads, like gettext's N_(). */
#define N_(s) s

typedef struct { const char *en, *es; } I18nPair;
extern const I18nPair I18N_ES[];
extern const int I18N_ES_COUNT;

#endif
