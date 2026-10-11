#include "report.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define ERRLOG_SIZE 20
#define ERRLOG_TEXT 200

static char errlog[ERRLOG_SIZE][ERRLOG_TEXT];
static int errlog_next, errlog_total;

void errlog_add(const char *kind, const char *detail) {
  Buf b = {0};
  char *line;
  buf_str(&b, kind);
  buf_str(&b, ": ");
  buf_str(&b, detail);
  line = buf_take(&b);
  if (strlen(line) >= ERRLOG_TEXT) line[ERRLOG_TEXT - 1] = 0;
  strcpy(errlog[errlog_next], line);
  free(line);
  errlog_next = (errlog_next + 1) % ERRLOG_SIZE;
  errlog_total++;
}

int errlog_count(void) { return errlog_total < ERRLOG_SIZE ? errlog_total : ERRLOG_SIZE; }

void errlog_dump(Buf *out) {
  int i, n = errlog_count(), start = errlog_total < ERRLOG_SIZE ? 0 : errlog_next;
  for (i = 0; i < n; i++) {
    buf_str(out, errlog[(start + i) % ERRLOG_SIZE]);
    buf_str(out, "\n");
  }
}

static size_t match_ci(const char *s, const char *word) { /* length matched at s, or 0 */
  size_t i;
  for (i = 0; word[i]; i++)
    if (tolower((unsigned char)s[i]) != tolower((unsigned char)word[i])) return 0;
  return i;
}

static size_t token_len(const char *s) { /* key-like characters */
  size_t n = 0;
  while (isalnum((unsigned char)s[n]) || s[n] == '-' || s[n] == '_' || s[n] == '.') n++;
  return n;
}

char *redact(const char *text, const char *const *names, const char *const *labels, int n) {
  Buf b = {0};
  const char *p = text;
  while (*p) {
    size_t len;
    int i, done = 0;
    if (!strncmp(p, "sk-", 3) && token_len(p + 3) >= 8) {
      buf_str(&b, "sk-<redactado>");
      p += 3 + token_len(p + 3);
      continue;
    }
    if ((len = match_ci(p, "bearer ")) != 0 && token_len(p + len) >= 8) {
      buf_add(&b, p, len);
      buf_str(&b, "<redactado>");
      p += len + token_len(p + len);
      continue;
    }
    for (i = 0; i < n && !done; i++)
      if (names[i] && strlen(names[i]) >= 2 && (len = match_ci(p, names[i])) != 0) {
        buf_str(&b, labels[i]);
        p += len;
        done = 1;
      }
    if (!done) buf_add(&b, p++, 1);
  }
  return buf_take(&b);
}

static void field(Buf *b, const char *key, const char *value, int first) {
  char *e = jesc(value ? value : "");
  if (!first) buf_str(b, ",");
  buf_str(b, "\"");
  buf_str(b, key);
  buf_str(b, "\":\"");
  buf_str(b, e);
  buf_str(b, "\"");
  free(e);
}

char *report_json(const char *install_id, const char *version, const char *os, const char *title,
                  const char *public_text, const char *full_text) {
  Buf b = {0};
  buf_str(&b, "{");
  field(&b, "install_id", install_id, 1);
  field(&b, "version", version, 0);
  field(&b, "os", os, 0);
  field(&b, "title", title, 0);
  field(&b, "public", public_text, 0);
  field(&b, "full", full_text, 0);
  buf_str(&b, "}");
  return buf_take(&b);
}

char *event_json(const char *install_id, const char *version, const char *os, const char *name,
                 const char *data_json) {
  Buf b = {0};
  buf_str(&b, "{");
  field(&b, "install_id", install_id, 1);
  field(&b, "version", version, 0);
  field(&b, "os", os, 0);
  field(&b, "name", name, 0);
  buf_str(&b, ",\"data\":");
  buf_str(&b, data_json && *data_json ? data_json : "null");
  buf_str(&b, "}");
  return buf_take(&b);
}
