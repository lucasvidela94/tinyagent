#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "machine.h"
#include "report.h"
#include "settings.h"
#include "sys.h"
#include "telemetry.h"
#include "tui.h"
#include "util.h"

static int enabled;
static char install_id[48];

static void new_install_id(void) {
  unsigned char r[12];
  int i;
  sys_random(r, sizeof r);
  for (i = 0; i < 12; i++) snprintf(install_id + 2 * i, 3, "%02x", r[i]);
  settings_set("telemetry", "install_id", install_id);
}

void telemetry_init(void) {
  char v[16];
  if (!settings_get("telemetry", "install_id", install_id, sizeof install_id) || strlen(install_id) < 8)
    new_install_id();
  /* Beta default: on while 0.x unless the user turned it off. */
  enabled = settings_get("telemetry", "enabled", v, sizeof v) ? atoi(v) != 0 : TINYSHELL_VERSION[0] == '0';
  if (enabled && !settings_get("telemetry", "noticed", v, sizeof v)) {
    tui_note("Beta: TinyShell envía datos técnicos de uso y errores (versión, sistema, modelo, códigos de\n"
             "error; nunca lo que escribís ni los comandos). Ver: tinyshell telemetria --ver ·\n"
             "apagar: tinyshell telemetria off");
    settings_set("telemetry", "noticed", "1");
  }
}

int telemetry_enabled(void) { return enabled; }
const char *telemetry_install_id(void) { return install_id; }

static void log_line(const char *json) {
  Buf path = {0}, line = {0};
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char stamp[48] = "";
  if (t) strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S ", t);
  buf_str(&line, stamp);
  buf_str(&line, json);
  buf_str(&line, SYS_EOL);
  if (sys_data_path("telemetria.log", &path)) sys_write_file(path.p, line.p, line.len, SYS_APPEND);
  free(path.p);
  free(line.p);
}

void telemetry_event(const char *name, const char *data_json) {
  Buf os = {0};
  char *json, *resp;
  if (!enabled) return;
  system_os(&os);
  json = event_json(install_id, TINYSHELL_VERSION, os.p, name, data_json);
  resp = http_post_json(RELAY_URL "/v1/event", json, 1, 4096);
  log_line(json);
  free(resp);
  free(json);
  free(os.p);
}

void telemetry_error(const char *kind, const char *code, const char *stage) {
  Buf d = {0};
  char *ek = jesc(kind), *ec = jesc(code), *es = jesc(stage), detail[200];
  snprintf(detail, sizeof detail, "%s (%s)", code, stage);
  errlog_add(kind, detail);
  buf_str(&d, "{\"kind\":\"");
  buf_str(&d, ek);
  buf_str(&d, "\",\"code\":\"");
  buf_str(&d, ec);
  buf_str(&d, "\",\"stage\":\"");
  buf_str(&d, es);
  buf_str(&d, "\"}");
  telemetry_event("error", d.p);
  free(d.p);
  free(ek);
  free(ec);
  free(es);
}

static void show_log(void) {
  Buf path = {0};
  char *tail = sys_data_path("telemetria.log", &path) ? sys_read_file(path.p, 4095, NULL) : NULL;
  if (tail) say("Últimos eventos enviados (completo en %s):\n%s", path.p, tail);
  else say("Todavía no se envió nada.\n");
  free(tail);
  free(path.p);
}

int telemetry_command(const char *arg) {
  if (arg && (!strcmp(arg, "off") || !strcmp(arg, "on"))) {
    int on = !strcmp(arg, "on");
    settings_set("telemetry", "enabled", on ? "1" : "0");
    settings_set("telemetry", "noticed", "1");
    say(on ? "Telemetría activada. Gracias.\n" : "Telemetría desactivada.\n");
    return 0;
  }
  if (arg && !strcmp(arg, "--ver")) {
    show_log();
    return 0;
  }
  telemetry_init();
  say("Telemetría: %s · id de instalación %s\nUso: tinyshell telemetria on | off | --ver\n",
      enabled ? "activada" : "desactivada", install_id);
  return 0;
}
