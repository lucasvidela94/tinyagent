/* Problem reports and telemetry payloads. Portable C.
 * Builds what the client sends to the relay; the Win32 side collects and posts it. */
#ifndef TINYSHELL_REPORT_H
#define TINYSHELL_REPORT_H

#include "util.h"

/* In-memory log of the last errors of this run, for reports. */
void errlog_add(const char *kind, const char *detail);
void errlog_dump(Buf *out); /* one "kind: detail" per line, oldest first */
int errlog_count(void);

/* Copy of text with secrets and identifying data replaced: "sk-..." keys and "Bearer x"
 * tokens always; each non-empty entry of `names` (hostname, user name, home folder)
 * replaced by the matching entry of `labels`. Matching ignores ASCII case. Caller frees. */
char *redact(const char *text, const char *const *names, const char *const *labels, int n);

/* {"install_id":..,"version":..,"os":..,"title":..,"public":..,"full":..} */
char *report_json(const char *install_id, const char *version, const char *os, const char *title,
                  const char *public_text, const char *full_text);

/* {"install_id":..,"version":..,"os":..,"name":..,"data":<data_json or null>} */
char *event_json(const char *install_id, const char *version, const char *os, const char *name,
                 const char *data_json);

#endif
