/* What depends on the platform's shell, as data: the local risk rules and the shell
 * guidance in the system prompt. One table per shell (dialect_cmd.c, dialect_sh.c); the
 * platform says which one applies (machine.h). Portable C, so both are host-tested. */
#ifndef TINYSHELL_DIALECT_H
#define TINYSHELL_DIALECT_H

/* Fires when `word` appears as a word and, if `flags` is set, any one of its
 * space-separated flags too; a flag ending in '*' matches as a prefix ("-i*": "-i.bak"). */
typedef struct { const char *word, *flags, *reason; } RiskRule;

/* Fires when `text` appears anywhere in the raw command. */
typedef struct { const char *text, *reason; } RiskMark;

typedef struct {
  const RiskMark *marks; /* checked first, on the raw text */
  const RiskRule *rules;
  int nmarks, nrules;
  const char *split;  /* characters that separate words */
  char flag;          /* flag prefix kept on its word ('/' for cmd); 0 if none */
  int fold_case;      /* 1: match ignoring case (cmd); 0: "-P" and "-p" differ (sh) */
  int command_words;  /* 1: a rule's word must be the command of a | ; & segment (its
                         basename before any '.'), not any word: "ls /etc/init.d" is safe */

  /* System prompt pieces (English, see docs/SYSTEM_PROMPT.md). */
  const char *os;      /* "Windows" */
  const char *machine; /* "Windows PC" */
  const char *shell;   /* "cmd.exe" */
  const char *scripts; /* "batch scripts" */
  const char *no_scan; /* examples of scans that are too heavy for the machine */
  const char *tools;   /* the built-in tools bullet */
  const char *changes; /* what counts as changing the system */
} Dialect;

extern const Dialect DIALECT_CMD, DIALECT_SH;

#endif
