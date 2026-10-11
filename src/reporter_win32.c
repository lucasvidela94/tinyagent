#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "json.h"
#include "policy.h"
#include "prompt.h"
#include "report.h"
#include "reporter.h"
#include "settings.h"
#include "telemetry.h"
#include "tui.h"

#define MAX_FULL (200 * 1024)

/* Hostname, user name and home folder, to keep them out of the public summary. */
static void identity(char host[64], char user[64], char home[MAX_PATH * 3]) {
  wchar_t w[MAX_PATH];
  DWORD n = 64;
  host[0] = user[0] = home[0] = 0;
  if (GetComputerNameW(w, &n)) WideCharToMultiByte(CP_UTF8, 0, w, -1, host, 64, NULL, NULL);
  n = 64;
  if (GetUserNameW(w, &n)) WideCharToMultiByte(CP_UTF8, 0, w, -1, user, 64, NULL, NULL);
  if (GetEnvironmentVariableW(L"USERPROFILE", w, MAX_PATH))
    WideCharToMultiByte(CP_UTF8, 0, w, -1, home, MAX_PATH * 3, NULL, NULL);
}

static void summary(Buf *b, const Session *s, const char *what) {
  buf_str(b, "Versión: " TINYSHELL_VERSION "\nSistema: ");
  system_details(b);
  if (s) {
    buf_str(b, "\nProveedor: ");
    buf_str(b, s->provider.name);
    buf_str(b, " · ");
    buf_str(b, s->provider.model);
  }
  buf_str(b, "\nQué pasó: ");
  buf_str(b, what);
  buf_str(b, "\nErrores recientes:\n");
  if (errlog_count()) errlog_dump(b);
  else buf_str(b, "(ninguno registrado)\n");
}

static void conversation(Buf *b, const Session *s) {
  int i;
  buf_str(b, "\n--- conversación (JSON por mensaje) ---\n");
  for (i = 1; i < s->history.n; i++) { /* m[0] is the system prompt, already known */
    buf_str(b, s->history.m[i].json);
    buf_str(b, "\n");
  }
}

static int save_file(const char *text, wchar_t *path, size_t cap) {
  SYSTEMTIME t;
  char name[64];
  wchar_t dir[MAX_PATH];
  HANDLE f;
  DWORD done;
  GetLocalTime(&t);
  if (!settings_file("reportes", dir, MAX_PATH)) return 0;
  CreateDirectoryW(dir, NULL);
  snprintf(name, sizeof name, "reportes\\reporte-%04d%02d%02d-%02d%02d%02d.txt", t.wYear, t.wMonth, t.wDay, t.wHour,
           t.wMinute, t.wSecond);
  if (!settings_file(name, path, cap)) return 0;
  f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) return 0;
  WriteFile(f, text, (DWORD)strlen(text), &done, NULL);
  CloseHandle(f);
  return 1;
}

static void show_result(const char *resp) {
  Json j = {0};
  char *id, *url, msg[300];
  if (!json_parse(&j, resp)) {
    tui_note("reporte enviado");
    return;
  }
  id = json_str(&j, json_key(&j, 0, "id"));
  url = json_str(&j, json_key(&j, 0, "issue_url"));
  snprintf(msg, sizeof msg, "reporte #%s enviado%s%s · gracias", id, *url ? ": " : "", url);
  tui_note(msg);
  free(id);
  free(url);
  json_free(&j);
}

int reporter_run(const Session *s) {
  char what[1024], answer[16], host[64], user[64], home[MAX_PATH * 3], fname[MAX_PATH * 3], msg[MAX_PATH * 3 + 128];
  const char *names[3], *labels[3] = {"%USERPROFILE%", "<pc>", "<usuario>"};
  Buf pub = {0}, full = {0}, both = {0}, os = {0};
  char *public_text, *full_text, *json, *resp;
  wchar_t path[MAX_PATH];
  int saved;
  say("\nContá qué pasó en una línea (qué hiciste, qué esperabas, qué salió): ");
  if (read_line(what, sizeof what, 1) != LINE_OK || !*what) {
    tui_note("cancelado");
    return 1;
  }
  identity(host, user, home);
  names[0] = home;
  names[1] = host;
  names[2] = user;
  summary(&pub, s, what);
  public_text = redact(pub.p, names, labels, 3);
  summary(&full, s, what);
  if (s && s->history.n > 1) {
    say("¿Incluir la conversación de esta sesión (ayuda mucho a arreglarlo)? [S/n] ");
    if (read_line(answer, sizeof answer, 1) != LINE_OK || (*answer != 'n' && *answer != 'N')) conversation(&full, s);
  }
  full_text = redact(full.p, NULL, NULL, 0); /* private copy: only secrets removed */
  if (strlen(full_text) > MAX_FULL) memmove(full_text, full_text + strlen(full_text) - MAX_FULL, MAX_FULL + 1);
  buf_str(&both, "=== resumen público (issue en GitHub) ===\n");
  buf_str(&both, public_text);
  buf_str(&both, "\n=== informe completo (privado, solo para el desarrollador) ===\n");
  buf_str(&both, full_text);
  saved = save_file(both.p, path, MAX_PATH);
  fname[0] = 0;
  if (saved) WideCharToMultiByte(CP_UTF8, 0, path, -1, fname, sizeof fname, NULL, NULL);
  say("\n");
  tui_note("Resumen público (se publica como issue en GitHub):");
  tui_line("", public_text, COLOR_NORMAL);
  snprintf(msg, sizeof msg, "Además se envía el informe completo, privado (%lu KB)%s%s", (unsigned long)(strlen(full_text) / 1024 + 1),
           saved ? ". Todo lo que se envía está en:\n" : "", fname);
  tui_note(msg);
  console_flush_input();
  say("¿Enviar a los desarrolladores? [s/N] ");
  if (read_line(answer, sizeof answer, 1) == LINE_OK && is_yes(answer)) {
    system_os(&os);
    json = report_json(telemetry_install_id(), TINYSHELL_VERSION, os.p, what, public_text, full_text);
    if ((resp = http_post_json(RELAY_URL "/v1/report", json, 0, 4096)) != NULL) show_result(resp);
    else {
      snprintf(msg, sizeof msg, "No se pudo enviar. Mandá el archivo de arriba a " REPORT_EMAIL);
      tui_error(msg);
    }
    free(resp);
    free(json);
  } else {
    tui_note(saved ? "no se envió nada; el informe quedó guardado en el archivo de arriba" : "no se envió nada");
  }
  free(os.p);
  free(pub.p);
  free(full.p);
  free(both.p);
  free(public_text);
  free(full_text);
  return 0;
}
