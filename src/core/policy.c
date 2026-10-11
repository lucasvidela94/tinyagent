#include "policy.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

const char *command_problem(const ToolCall *tc) {
  const unsigned char *p;
  if (strcmp(tc->name, "shell")) return "Herramienta no permitida. No se ejecutó nada.";
  if (!*tc->command) return "Faltó el comando (argumentos ausentes o JSON inválido). No se ejecutó nada.";
  if (strlen(tc->command) > MAX_COMMAND) return "Comando demasiado largo. No se ejecutó nada.";
  for (p = (const unsigned char *)tc->command; *p; p++)
    if (*p < 0x20 || *p == 0x7F) return "El comando tiene saltos de línea o caracteres de control. No se ejecutó nada.";
  return NULL;
}

/* text is " word word "; a pattern ending in '*' matches a word's prefix. */
static int has_word(const char *text, const char *w, size_t n) {
  char pat[40];
  int prefix = n && w[n - 1] == '*';
  if (prefix) n--;
  if (n + 3 > sizeof pat) return 0;
  pat[0] = ' ';
  memcpy(pat + 1, w, n);
  pat[n + 1] = prefix ? 0 : ' ';
  pat[n + 2] = 0;
  return strstr(text, pat) != NULL;
}

/* words: where the rule's word must appear; all: where its flags may appear. */
static int rule_fires(const RiskRule *rule, const char *words, const char *all) {
  const char *f = rule->flags;
  if (!has_word(words, rule->word, strlen(rule->word))) return 0;
  if (!f) return 1;
  while (*f) {
    size_t n = strcspn(f, " ");
    if (has_word(all, f, n)) return 1;
    f += n;
    while (*f == ' ') f++;
  }
  return 0;
}

static const char *first_rule(const Dialect *d, const char *words, const char *all) {
  int r;
  for (r = 0; r < d->nrules; r++)
    if (rule_fires(&d->rules[r], words, all)) return d->rules[r].reason;
  return NULL;
}

/* " word word " in lowercase, split at the dialect's separators; a flag prefix starts a word. */
static void words_of(const Dialect *d, const char *s, size_t n, Buf *out) {
  size_t i;
  buf_str(out, " ");
  for (i = 0; i < n; i++) {
    char c = d->fold_case ? (char)tolower((unsigned char)s[i]) : s[i];
    if (d->flag && c == d->flag) {
      char f[3] = {' ', c, 0};
      buf_str(out, f);
    } else if (strchr(d->split, c)) buf_str(out, " ");
    else buf_add(out, &c, 1);
  }
  buf_str(out, " ");
}

/* Wrappers that run the next word as the command (sudo has its own rule). */
static int is_wrapper(const char *w, size_t n) {
  static const char *const WRAP[] = {"env", "nohup", "nice", "time", "exec", "command", "xargs", "busybox"};
  size_t i;
  for (i = 0; i < sizeof WRAP / sizeof WRAP[0]; i++)
    if (strlen(WRAP[i]) == n && !strncmp(w, WRAP[i], n)) return 1;
  return 0;
}

/* The command of a segment: past VAR=value and wrappers, its basename before any '.'. */
static void command_of(const char *all, Buf *out) {
  const char *p = all;
  for (;;) {
    size_t n;
    const char *base, *dot;
    while (*p == ' ') p++;
    n = strcspn(p, " ");
    if (!n) return;
    if (memchr(p, '=', n) || is_wrapper(p, n)) {
      p += n;
      continue;
    }
    for (base = p + n; base > p && base[-1] != '/'; base--) {}
    dot = memchr(base, '.', (size_t)(p + n - base));
    buf_str(out, " ");
    buf_add(out, base, (size_t)((dot ? dot : p + n) - base));
    buf_str(out, " ");
    return;
  }
}

const char *risk_reason(const Dialect *d, const char *cmd) {
  const char *found = NULL, *seg = cmd;
  int m;
  for (m = 0; m < d->nmarks; m++)
    if (strstr(cmd, d->marks[m].text)) return d->marks[m].reason;
  if (!d->command_words) {
    Buf all = {0};
    words_of(d, cmd, strlen(cmd), &all);
    found = first_rule(d, all.p, all.p);
    free(all.p);
    return found;
  }
  while (!found) { /* each | ; & ( ) segment on its own */
    size_t n = strcspn(seg, "|;&()");
    Buf all = {0}, word = {0};
    words_of(d, seg, n, &all);
    command_of(all.p, &word);
    if (word.p) found = first_rule(d, word.p, all.p);
    free(all.p);
    free(word.p);
    if (!seg[n]) break;
    seg += n + 1;
  }
  return found;
}

int is_yes(const char *answer) {
  char a[8];
  size_t n = 0;
  while (*answer == ' ' || *answer == '\t') answer++;
  while (*answer && n < sizeof a - 1) a[n++] = (char)tolower((unsigned char)*answer++);
  a[n] = 0;
  trim_line(a);
  if (*answer) return 0; /* longer than any accepted answer */
  return !strcmp(a, "s") || !strcmp(a, "si") || !strcmp(a, "s\xc3\xad") || !strcmp(a, "s\xc3\x8d");
}
