#include "policy.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "i18n.h"
#include "util.h"

/* Control characters other than those in `allowed`. */
static int has_control(const char *s, const char *allowed) {
  const unsigned char *p;
  for (p = (const unsigned char *)s; *p; p++)
    if ((*p < 0x20 || *p == 0x7F) && !strchr(allowed, *p)) return 1;
  return 0;
}

/* Each option is one menu row; the question may span lines. */
static const char *question_problem(const ToolCall *tc) {
  int i;
  if (!*tc->question || tc->noptions < 2 || tc->noptions > MAX_OPTIONS)
    return N_("The question needs its text and 2 to 8 options. Nothing was asked.");
  if (strlen(tc->question) > MAX_QUESTION || has_control(tc->question, "\n"))
    return N_("The question is too long or has control characters. Nothing was asked.");
  for (i = 0; i < tc->noptions; i++)
    if (!*tc->options[i] || strlen(tc->options[i]) > MAX_OPTION || has_control(tc->options[i], ""))
      return N_("An option is empty, too long or has control characters. Nothing was asked.");
  return NULL;
}

const char *tool_problem(const ToolCall *tc) {
  if (!strcmp(tc->name, "question")) return question_problem(tc);
  if (strcmp(tc->name, "shell")) return N_("Tool not allowed. Nothing was run.");
  if (!*tc->command) return N_("The command is missing (no arguments or invalid JSON). Nothing was run.");
  if (strlen(tc->command) > MAX_COMMAND) return N_("Command too long. Nothing was run.");
  if (has_control(tc->command, "")) return N_("The command has line breaks or control characters. Nothing was run.");
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

static const RiskRule *first_rule(const RiskRule *rules, int n, const char *words, const char *all) {
  int r;
  for (r = 0; r < n; r++)
    if (rule_fires(&rules[r], words, all)) return &rules[r];
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

/* The command of a segment: past VAR=value and wrappers, its basename before any '.'.
 * After sudo or doas the command they run (past their flags) comes out too, since sudo
 * has a rule of its own: "sudo dd" yields " sudo  dd ". */
static void command_of(const char *all, Buf *out) {
  const char *p = all;
  int elevated = 0;
  for (;;) {
    size_t n, len;
    const char *base, *dot;
    while (*p == ' ') p++;
    n = strcspn(p, " ");
    if (!n) return;
    if (memchr(p, '=', n) || is_wrapper(p, n) || (elevated && *p == '-')) {
      p += n;
      continue;
    }
    for (base = p + n; base > p && base[-1] != '/'; base--) {}
    dot = memchr(base, '.', (size_t)(p + n - base));
    len = (size_t)((dot ? dot : p + n) - base);
    buf_str(out, " ");
    buf_add(out, base, len);
    buf_str(out, " ");
    if (elevated || len != 4 || (strncmp(base, "sudo", 4) && strncmp(base, "doas", 4))) return;
    elevated = 1;
    p += n;
  }
}

/* The first of rules[0..n) that fires on cmd, read the dialect's way. */
static const RiskRule *first_match(const Dialect *d, const RiskRule *rules, int n, const char *cmd) {
  const RiskRule *found = NULL;
  const char *seg = cmd;
  if (!d->command_words) {
    Buf all = {0};
    words_of(d, cmd, strlen(cmd), &all);
    found = first_rule(rules, n, all.p, all.p);
    free(all.p);
    return found;
  }
  while (!found) { /* each | ; & ( ) segment on its own */
    size_t len = strcspn(seg, "|;&()");
    Buf all = {0}, word = {0};
    words_of(d, seg, len, &all);
    command_of(all.p, &word);
    if (word.p) found = first_rule(rules, n, word.p, all.p);
    free(all.p);
    free(word.p);
    if (!seg[len]) break;
    seg += len + 1;
  }
  return found;
}

static int is_nul(const char *r) {
  return (r[0] | 0x20) == 'n' && (r[1] | 0x20) == 'u' && (r[2] | 0x20) == 'l' && !isalnum((unsigned char)r[3]) &&
         r[3] != '.';
}

/* A copy with redirections to the null device blanked ("2>nul", ">nul", "2>/dev/null",
 * "2>&1"): they hide output and touch no file, so they add no risk. */
static char *without_null_redirects(const char *cmd) {
  char *s = xstrdup(cmd), *p;
  for (p = s; *p; p++) {
    char *r = p + (*p >= '0' && *p <= '9' && p[1] == '>'), *end = NULL;
    if (*r != '>') continue;
    r += r[1] == '>' ? 2 : 1;
    if (*r == '&' && r[1] >= '0' && r[1] <= '9') end = r + 2;
    else {
      while (*r == ' ') r++;
      if (is_nul(r)) end = r + 3;
      else if (!strncmp(r, "/dev/null", 9) && !isalnum((unsigned char)r[9])) end = r + 9;
    }
    if (end) {
      memset(p, ' ', (size_t)(end - p));
      p = end - 1;
    }
  }
  return s;
}

const char *risk_reason(const Dialect *d, const char *cmd) {
  const RiskRule *r = NULL;
  const char *found = NULL;
  char *c = without_null_redirects(cmd);
  int m;
  for (m = 0; m < d->nmarks && !found; m++)
    if (strstr(c, d->marks[m].text)) found = d->marks[m].reason;
  if (!found && (r = first_match(d, d->rules, d->nrules, c)) != NULL) found = r->reason;
  free(c);
  return found;
}

/* Lowercase in place (cmd matches ignoring case). */
static void lower(char *s) {
  for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static int in_list(const char *w, const char *const *list) {
  for (; *list; list++)
    if (!strcmp(w, *list)) return 1;
  return 0;
}

int command_prefix(const Dialect *d, const char *cmd, char *out, size_t cap) {
  /* They run other commands, so "always" for them would approve anything. */
  static const char *const RUNS_OTHERS[] = {"cmd", "start", "call", "runas", "forfiles", "for", "if", "sh", "bash",
                                            "dash", "zsh", "env", "sudo", "doas", "su", "xargs", "nohup", "nice",
                                            "exec", "eval", "busybox", "find", "time", "command", "watch",
                                            "timeout", NULL};
  /* Tools whose second word says what they do: "reg query" is not "reg delete". */
  static const char *const SUBCOMMANDS[] = {"reg", "sc", "net", "netsh", "wmic", "schtasks", "git", "apt",
                                            "apt-get", "systemctl", "service", "dpkg", "pacman", "npm", "pnpm",
                                            "pip", "docker", "ip", "nmcli", NULL};
  char *c = without_null_redirects(cmd), w[2][32];
  const char *p = c;
  size_t n, len;
  int i, ok = 0;
  if (strpbrk(c, d->command_words ? ";&|<>`$()" : "&|<>^()") || risk_reason(d, cmd)) goto done;
  for (i = 0; i < 2; i++) { /* the first two words */
    while (*p == ' ' || *p == '\t') p++;
    n = strcspn(p, " \t");
    if (n >= sizeof w[i]) goto done;
    memcpy(w[i], p, n);
    w[i][n] = 0;
    if (d->fold_case) lower(w[i]);
    p += n;
  }
  len = strlen(w[0]);
  if (d->fold_case && len > 4 && !strcmp(w[0] + len - 4, ".exe")) w[0][len - 4] = 0;
  /* a path or quotes could name any program; a bare name is looked up the usual way */
  if (!*w[0] || strpbrk(w[0], "/\\\"'") || in_list(w[0], RUNS_OTHERS)) goto done;
  if (!in_list(w[0], SUBCOMMANDS)) ok = snprintf(out, cap, "%s", w[0]) < (int)cap;
  else if (*w[1] && strspn(w[1], "abcdefghijklmnopqrstuvwxyz-") == strlen(w[1]))
    ok = snprintf(out, cap, "%s %s", w[0], w[1]) < (int)cap;
done:
  free(c);
  return ok;
}

int denied_by(const Dialect *d, const char *rules, const char *cmd, char *out, size_t cap) {
  const char *r = rules;
  for (;;) {
    size_t n = strcspn(r, ","), i;
    char rule[64], *word = rule, *sp;
    RiskRule rr = {NULL, NULL, NULL};
    if (n < sizeof rule) {
      memcpy(rule, r, n);
      rule[n] = 0;
      trim_line(rule);
      while (*word == ' ' || *word == '\t') word++;
      if (d->fold_case)
        for (i = 0; word[i]; i++) word[i] = (char)tolower((unsigned char)word[i]);
      snprintf(out, cap, "%s", word);
      if ((sp = strchr(word, ' ')) != NULL) { /* "del /s": the word, then its flags */
        *sp++ = 0;
        while (*sp == ' ') sp++;
        rr.flags = sp;
      }
      rr.word = word;
      if (*word && first_match(d, &rr, 1, cmd)) return 1;
    }
    if (!r[n]) return 0;
    r += n + 1;
  }
}

int is_yes(const char *answer) {
  char a[8];
  size_t n = 0;
  while (*answer == ' ' || *answer == '\t') answer++;
  while (*answer && n < sizeof a - 1) a[n++] = (char)tolower((unsigned char)*answer++);
  a[n] = 0;
  trim_line(a);
  if (*answer) return 0; /* longer than any accepted answer */
  return !strcmp(a, "s") || !strcmp(a, "si") || !strcmp(a, "s\xc3\xad") || !strcmp(a, "s\xc3\x8d") || !strcmp(a, "y") ||
         !strcmp(a, "yes");
}
