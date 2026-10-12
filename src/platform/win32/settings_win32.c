#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "i18n.h"
#include "settings.h"
#include "sys.h"

static wchar_t path_w[MAX_PATH];
static char path_utf8[MAX_PATH * 3];

/* The provider section of a new config.ini; the comment above it is written in the
 * interface language (see write_template). */
static const char *SECTIONS =
    "\r\n"
    "[provider]\r\n"
    "name = opencode-go\r\n"
    "url = https://opencode.ai/zen/go/v1/chat/completions\r\n"
    "model = deepseek-v4.1-flash\r\n"
    "session_header = x-opencode-session\r\n"
    "\r\n"
    "[ui]\r\n"
    "language = auto\r\n";

/* <data dir>/config.ini (see sys_data_path), kept wide for the profile API. */
static int resolve_path(void) {
  Buf b = {0};
  int ok = sys_data_path("config.ini", &b) && b.len < sizeof path_utf8 &&
           MultiByteToWideChar(CP_UTF8, 0, b.p, -1, path_w, MAX_PATH) != 0;
  if (ok) strcpy(path_utf8, b.p); /* length checked above */
  else path_w[0] = 0;
  free(b.p);
  return ok;
}

static void write_template(void) {
  DWORD done;
  const char *head = tr("; TinyShell: the model provider. No secrets here: API keys go to the Windows\r\n"
                        "; Credential Manager (one per 'name'). Any OpenAI-compatible chat-completions service.\r\n"
                        ";\r\n"
                        "; Examples:\r\n"
                        ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\r\n"
                        ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\r\n"
                        ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\r\n"
                        ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: the key travels unencrypted)\r\n"
                        "; For providers other than OpenCode Go, leave session_header empty.\r\n"
                        "; [ui] language: es, en or auto (the system's language).\r\n");
  HANDLE f = CreateFileW(path_w, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) return;
  WriteFile(f, head, (DWORD)strlen(head), &done, NULL);
  WriteFile(f, SECTIONS, (DWORD)strlen(SECTIONS), &done, NULL);
  CloseHandle(f);
}

static void apply(Provider *p, const char *key) {
  wchar_t wkey[32], wval[600];
  char val[1800], msg[160];
  const char *err;
  MultiByteToWideChar(CP_UTF8, 0, key, -1, wkey, 32);
  /* "\x01" marks "key absent", so an empty value (e.g. session_header =) still applies. */
  GetPrivateProfileStringW(L"provider", wkey, L"\x01", wval, 600, path_w);
  if (wval[0] == 1) return;
  if (!WideCharToMultiByte(CP_UTF8, 0, wval, -1, val, sizeof val, NULL, NULL)) return;
  if ((err = provider_set(p, key, val)) != NULL) {
    snprintf(msg, sizeof msg, tr("config.ini: %s (the default is used)"), tr(err));
    console_write(msg, COLOR_DANGER);
    say("\n");
  }
}

void settings_load(Provider *p) {
  provider_defaults(p);
  if (!resolve_path()) return;
  if (GetFileAttributesW(path_w) == INVALID_FILE_ATTRIBUTES) write_template();
  apply(p, "name");
  apply(p, "url");
  apply(p, "model");
  apply(p, "session_header");
}

static int widen(const char *s, wchar_t *w, int cap) { return MultiByteToWideChar(CP_UTF8, 0, s, -1, w, cap) != 0; }

int settings_set(const char *section, const char *key, const char *value) {
  wchar_t ws[64], wk[64], wv[600];
  if (!path_w[0] && !resolve_path()) return 0;
  if (!widen(section, ws, 64) || !widen(key, wk, 64) || !widen(value, wv, 600)) return 0;
  return WritePrivateProfileStringW(ws, wk, wv, path_w) != 0;
}

int settings_get(const char *section, const char *key, char *out, size_t cap) {
  wchar_t ws[64], wk[64], wv[600];
  out[0] = 0;
  if (!path_w[0] && !resolve_path()) return 0;
  if (!widen(section, ws, 64) || !widen(key, wk, 64)) return 0;
  GetPrivateProfileStringW(ws, wk, L"\x01", wv, 600, path_w);
  if (wv[0] == 1) return 0;
  return WideCharToMultiByte(CP_UTF8, 0, wv, -1, out, (int)cap, NULL, NULL) != 0;
}

static int save(const wchar_t *key, const char *value) {
  wchar_t w[600];
  if (!path_w[0] || !widen(value, w, 600)) return 0;
  return WritePrivateProfileStringW(L"provider", key, w, path_w) != 0;
}

int settings_save_model(const char *model) { return save(L"model", model); }

int settings_save_provider(const Provider *p) {
  if (!path_w[0] && !resolve_path()) return 0;
  return save(L"name", p->name) && save(L"url", p->url) && save(L"model", p->model) &&
         save(L"session_header", p->session_header);
}

const char *settings_path(void) { return path_utf8[0] ? path_utf8 : "(sin ruta)"; }
