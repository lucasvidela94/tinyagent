#include "posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "i18n.h"
#include "ini.h"
#include "settings.h"
#include "sys.h"

#define MAX_CONFIG (64 * 1024)

static char path[4096];

/* The provider section of a new config.ini; the comment above it is written in the
 * interface language (see write_template). */
static const char *SECTIONS = "\n"
                              "[provider]\n"
                              "name = opencode-go\n"
                              "url = https://opencode.ai/zen/go/v1/chat/completions\n"
                              "model = deepseek-v4.1-flash\n"
                              "session_header = x-opencode-session\n"
                              "\n"
                              "[ui]\n"
                              "language = auto\n"
                              "\n"
                              "[deny]\n"
                              "commands =\n";

static void write_template(void) {
  Buf b = {0};
  buf_str(&b, tr("; TinyShell: the model provider. No secrets here: API keys go to\n"
                 "; ~/.config/tinyshell/credentials (mode 0600, one per 'name'). Any OpenAI-compatible\n"
                 "; chat-completions service.\n"
                 ";\n"
                 "; Examples:\n"
                 ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\n"
                 ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\n"
                 ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\n"
                 ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: the key travels unencrypted)\n"
                 "; For providers other than OpenCode Go, leave session_header empty.\n"
                 "; [ui] language: es, en or auto (the system's language).\n"
                 "; [deny] commands: refused without asking, comma-separated, a command and optional flags.\n"
                 ";   Example: commands = dd, mkfs*, rm -r*, shutdown\n"));
  buf_str(&b, SECTIONS);
  sys_write_file(path, b.p, b.len, 0);
  free(b.p);
}

static int resolve_path(void) {
  Buf b = {0};
  int ok;
  if (path[0]) return 1;
  ok = sys_data_path("config.ini", &b) && b.len < sizeof path;
  if (ok) memcpy(path, b.p, b.len + 1); /* length checked above */
  free(b.p);
  return ok;
}

static char *read_config(void) { return resolve_path() ? sys_read_file(path, MAX_CONFIG, NULL) : NULL; }

static void apply(Provider *p, const char *text, const char *key) {
  char val[1800], msg[160];
  const char *err;
  if (!ini_get(text, "provider", key, val, sizeof val)) return;
  if ((err = provider_set(p, key, val)) != NULL) {
    snprintf(msg, sizeof msg, tr("config.ini: %s (the default is used)"), tr(err));
    console_write(msg, COLOR_DANGER);
    say("\n");
  }
}

void settings_load(Provider *p) {
  char *text;
  provider_defaults(p);
  if (!resolve_path()) return;
  if (!sys_exists(path)) write_template();
  if (!(text = sys_read_file(path, MAX_CONFIG, NULL))) return;
  apply(p, text, "name");
  apply(p, text, "url");
  apply(p, text, "model");
  apply(p, text, "session_header");
  free(text);
}

int settings_get(const char *section, const char *key, char *out, size_t cap) {
  char *text = read_config();
  int ok = text && ini_get(text, section, key, out, cap);
  if (!ok) out[0] = 0;
  free(text);
  return ok;
}

int settings_set(const char *section, const char *key, const char *value) {
  char *text = read_config();
  Buf next = {0};
  int ok = path[0] && ini_set(text, section, key, value, &next) && sys_write_file(path, next.p, next.len, 0);
  free(text);
  free(next.p);
  return ok;
}

int settings_save_model(const char *model) { return settings_set("provider", "model", model); }

int settings_save_provider(const Provider *p) {
  return settings_set("provider", "name", p->name) && settings_set("provider", "url", p->url) &&
         settings_set("provider", "model", p->model) && settings_set("provider", "session_header", p->session_header);
}

const char *settings_path(void) { return path[0] ? path : "(sin ruta)"; }
