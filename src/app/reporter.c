#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "i18n.h"
#include "json.h"
#include "policy.h"
#include "machine.h"
#include "report.h"
#include "reporter.h"
#include "sys.h"
#include "telemetry.h"
#include "tui.h"

#define MAX_FULL (200 * 1024)

static void summary(Buf *b, const Session *s, const char *what) {
  buf_str(b, "Version: " TINYSHELL_VERSION "\nSystem: ");
  system_details(b);
  if (s) {
    buf_str(b, "\nProvider: ");
    buf_str(b, s->provider.name);
    buf_str(b, " · ");
    buf_str(b, s->provider.model);
  }
  buf_str(b, "\nWhat happened: ");
  buf_str(b, what);
  buf_str(b, "\nRecent errors:\n");
  if (errlog_count()) errlog_dump(b);
  else buf_str(b, "(none logged)\n");
}

static void conversation(Buf *b, const Session *s) {
  int i;
  buf_str(b, "\n--- conversation (one JSON message per line) ---\n");
  for (i = 1; i < s->history.n; i++) { /* m[0] is the system prompt, already known */
    buf_str(b, s->history.m[i].json);
    buf_str(b, "\n");
  }
}

/* Saves the report under <data dir>/reports/ and appends its path to `path`. */
static int save_file(const char *text, Buf *path) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char name[64];
  if (!t || !strftime(name, sizeof name, "reports/report-%Y%m%d-%H%M%S.txt", t)) return 0;
  return sys_data_path(name, path) && sys_write_file(path->p, text, strlen(text), 0);
}

static void show_result(const char *resp) {
  Json j = {0};
  char *id, *url, msg[300];
  if (!json_parse(&j, resp)) {
    tui_note(tr("report sent"));
    return;
  }
  id = json_str(&j, json_key(&j, 0, "id"));
  url = json_str(&j, json_key(&j, 0, "issue_url"));
  snprintf(msg, sizeof msg, tr("report #%s sent%s%s \xc2\xb7 thanks"), id, *url ? ": " : "", url);
  tui_note(msg);
  free(id);
  free(url);
  json_free(&j);
}

int reporter_run(const Session *s) {
  char what[1024], answer[16], msg[256];
  const char *names[3], *labels[3] = {SYS_HOME_LABEL, "<host>", "<user>"};
  Buf pub = {0}, full = {0}, both = {0}, os = {0}, host = {0}, user = {0}, home = {0}, path = {0};
  char *public_text, *full_text, *json, *resp;
  int saved;
  say("%s", tr("\nDescribe in one line what happened (what you did, what you expected, what came out): "));
  if (read_line(what, sizeof what, 1) != LINE_OK || !*what) {
    tui_note(tr("cancelled"));
    return 1;
  }
  sys_identity(&host, &user, &home);
  names[0] = home.p;
  names[1] = host.p;
  names[2] = user.p;
  summary(&pub, s, what);
  public_text = redact(pub.p, names, labels, 3);
  summary(&full, s, what);
  if (s && s->history.n > 1) {
    say("%s", tr("Include this session's conversation (it helps a lot to fix it)? [Y/n] "));
    if (read_line(answer, sizeof answer, 1) != LINE_OK || (*answer != 'n' && *answer != 'N')) conversation(&full, s);
  }
  full_text = redact(full.p, NULL, NULL, 0); /* private copy: only secrets removed */
  if (strlen(full_text) > MAX_FULL) memmove(full_text, full_text + strlen(full_text) - MAX_FULL, MAX_FULL + 1);
  buf_str(&both, "=== public summary (GitHub issue) ===\n");
  buf_str(&both, public_text);
  buf_str(&both, "\n=== full report (private, for the developer only) ===\n");
  buf_str(&both, full_text);
  saved = save_file(both.p, &path);
  say("\n");
  tui_note(tr("Public summary (published as a GitHub issue):"));
  tui_line("", public_text, COLOR_NORMAL);
  snprintf(msg, sizeof msg, tr("The full report is also sent, privately (%lu KB)%s"),
           (unsigned long)(strlen(full_text) / 1024 + 1), saved ? tr(". Everything that is sent is in:\n") : "");
  if (saved) {
    Buf note = {0};
    buf_str(&note, msg);
    buf_str(&note, path.p);
    tui_note(note.p);
    free(note.p);
  } else {
    tui_note(msg);
  }
  console_flush_input();
  say("%s", tr("Send it to the developers? [y/N] "));
  if (read_line(answer, sizeof answer, 1) == LINE_OK && is_yes(answer)) {
    system_os(&os);
    json = report_json(telemetry_install_id(), TINYSHELL_VERSION, os.p, what, public_text, full_text);
    if ((resp = http_post_json(RELAY_URL "/v1/report", json, 0, 4096)) != NULL) show_result(resp);
    else {
      snprintf(msg, sizeof msg, tr("Could not send it. Email the file above to %s"), REPORT_EMAIL);
      tui_error(msg);
    }
    free(resp);
    free(json);
  } else {
    tui_note(saved ? tr("nothing was sent; the report is saved in the file above") : tr("nothing was sent"));
  }
  free(os.p);
  free(host.p);
  free(user.p);
  free(home.p);
  free(path.p);
  free(pub.p);
  free(full.p);
  free(both.p);
  free(public_text);
  free(full_text);
  return 0;
}
