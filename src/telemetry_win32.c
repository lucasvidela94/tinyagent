#include "win32.h"

#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "prompt.h"
#include "report.h"
#include "settings.h"
#include "telemetry.h"
#include "tui.h"
#include "util.h"

static int enabled;
static char install_id[48];

static void new_install_id(void) {
  HCRYPTPROV prov = 0;
  BYTE r[12];
  int i, ok = CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
              CryptGenRandom(prov, sizeof r, r);
  if (prov) CryptReleaseContext(prov, 0);
  if (!ok)
    for (i = 0; i < 12; i++) r[i] = (BYTE)(GetTickCount() >> (i % 4 * 8)) ^ (BYTE)(i * 53);
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
    tui_note("Beta: TinyShell envía datos técnicos de uso y errores (versión, Windows, modelo, códigos de\n"
             "error; nunca lo que escribís ni los comandos). Ver: tinyshell telemetria --ver ·\n"
             "apagar: tinyshell telemetria off");
    settings_set("telemetry", "noticed", "1");
  }
}

int telemetry_enabled(void) { return enabled; }
const char *telemetry_install_id(void) { return install_id; }

static void log_line(const char *json) {
  wchar_t path[MAX_PATH];
  HANDLE f;
  DWORD done;
  SYSTEMTIME t;
  char stamp[48];
  if (!settings_file("telemetria.log", path, MAX_PATH)) return;
  f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) return;
  GetLocalTime(&t);
  snprintf(stamp, sizeof stamp, "%04d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
           t.wSecond);
  WriteFile(f, stamp, (DWORD)strlen(stamp), &done, NULL);
  WriteFile(f, json, (DWORD)strlen(json), &done, NULL);
  WriteFile(f, "\r\n", 2, &done, NULL);
  CloseHandle(f);
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
  wchar_t path[MAX_PATH];
  char buf[4096];
  DWORD got = 0, size;
  HANDLE f;
  if (!settings_file("telemetria.log", path, MAX_PATH) ||
      (f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL)) ==
          INVALID_HANDLE_VALUE) {
    say("Todavía no se envió nada.\n");
    return;
  }
  size = GetFileSize(f, NULL);
  if (size > sizeof buf - 1) SetFilePointer(f, -(LONG)(sizeof buf - 1), NULL, FILE_END); /* last part */
  ReadFile(f, buf, sizeof buf - 1, &got, NULL);
  buf[got] = 0;
  CloseHandle(f);
  say("Últimos eventos enviados (completo en %%LOCALAPPDATA%%\\TinyShell\\telemetria.log):\n%s", buf);
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
