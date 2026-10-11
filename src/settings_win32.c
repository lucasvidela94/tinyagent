#include "win32.h"

#include <stdio.h>
#include <string.h>

#include "console.h"
#include "settings.h"

static wchar_t path_w[MAX_PATH];
static char path_utf8[MAX_PATH * 3];

static const char *TEMPLATE =
    "; TinyShell: proveedor del modelo. Sin secretos: las API keys van al Administrador de\r\n"
    "; credenciales (una por 'name'). Cualquier servicio compatible con OpenAI chat completions.\r\n"
    ";\r\n"
    "; Ejemplos:\r\n"
    ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\r\n"
    ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\r\n"
    ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\r\n"
    ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: la key viaja sin cifrar)\r\n"
    "; Para proveedores que no son OpenCode Go, dejá session_header vacío.\r\n"
    "\r\n"
    "[provider]\r\n"
    "name = opencode-go\r\n"
    "url = https://opencode.ai/zen/go/v1/chat/completions\r\n"
    "model = deepseek-v4.1-flash\r\n"
    "session_header = x-opencode-session\r\n";

/* %LOCALAPPDATA%\TinyShell\config.ini (XP: %USERPROFILE%\Local Settings\Application Data). */
static int resolve_path(void) {
  wchar_t base[MAX_PATH];
  DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
  if (!n || n >= MAX_PATH) {
    n = GetEnvironmentVariableW(L"USERPROFILE", base, MAX_PATH);
    if (!n || n + 40 >= MAX_PATH) return 0;
    lstrcatW(base, L"\\Local Settings\\Application Data");
  }
  if (lstrlenW(base) + 24 >= MAX_PATH) return 0;
  lstrcpyW(path_w, base);
  lstrcatW(path_w, L"\\TinyShell");
  CreateDirectoryW(path_w, NULL); /* fine if it already exists */
  lstrcatW(path_w, L"\\config.ini");
  WideCharToMultiByte(CP_UTF8, 0, path_w, -1, path_utf8, sizeof path_utf8, NULL, NULL);
  return 1;
}

static void write_template(void) {
  DWORD done;
  HANDLE f = CreateFileW(path_w, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) return;
  WriteFile(f, TEMPLATE, (DWORD)strlen(TEMPLATE), &done, NULL);
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
    snprintf(msg, sizeof msg, "config.ini: %s (se usa el valor por defecto)", err);
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

int settings_save_model(const char *model) {
  wchar_t w[128];
  if (!path_w[0] || !MultiByteToWideChar(CP_UTF8, 0, model, -1, w, 128)) return 0;
  return WritePrivateProfileStringW(L"provider", L"model", w, path_w) != 0;
}

const char *settings_path(void) { return path_utf8[0] ? path_utf8 : "(sin ruta)"; }
