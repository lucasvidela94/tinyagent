#include "provider.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

static const struct { const char *label; Provider p; } PRESETS[] = {
    {"OpenCode Go (el modelo se elige después)",
     {"opencode-go", "https://opencode.ai/zen/go/v1/chat/completions", "deepseek-v4.1-flash",
      "x-opencode-session"}}, /* OpenCode Go answers 400 without the session header */
};

int provider_preset_count(void) { return (int)(sizeof PRESETS / sizeof PRESETS[0]); }

const char *provider_preset_label(int i) {
  return i >= 0 && i < provider_preset_count() ? PRESETS[i].label : "";
}

void provider_preset(int i, Provider *p) { *p = PRESETS[i >= 0 && i < provider_preset_count() ? i : 0].p; }

void provider_defaults(Provider *p) { provider_preset(0, p); }

static int copy(char *dst, size_t cap, const char *src) {
  if (strlen(src) >= cap) return 0;
  strcpy(dst, src);
  return 1;
}

/* Letters, digits and a few separators: safe in a Credential Manager target and a footer. */
static int is_name(const char *s) {
  if (!*s) return 0;
  for (; *s; s++)
    if (!isalnum((unsigned char)*s) && !strchr("-_.", *s)) return 0;
  return 1;
}

/* RFC 7230 token characters only, so a value can never smuggle "\r\n" and more headers. */
static int is_header_name(const char *s) {
  for (; *s; s++)
    if (!isalnum((unsigned char)*s) && *s != '-') return 0;
  return 1;
}

static int is_printable(const char *s) {
  for (; *s; s++)
    if ((unsigned char)*s < 0x20 || *s == 0x7F || *s == ' ' || *s == '"') return 0;
  return 1;
}

const char *provider_set(Provider *p, const char *key, const char *value) {
  if (!strcmp(key, "name")) {
    if (!is_name(value)) return "name: usá letras, números, '-', '_' o '.'";
    return copy(p->name, sizeof p->name, value) ? NULL : "name: demasiado largo";
  }
  if (!strcmp(key, "url")) {
    if (strncmp(value, "https://", 8) && strncmp(value, "http://", 7)) return "url: debe empezar con https:// o http://";
    if (!is_printable(value)) return "url: tiene espacios o caracteres inválidos";
    return copy(p->url, sizeof p->url, value) ? NULL : "url: demasiado larga";
  }
  if (!strcmp(key, "model")) {
    if (!*value || !is_printable(value)) return "model: vacío o con espacios";
    return copy(p->model, sizeof p->model, value) ? NULL : "model: demasiado largo";
  }
  if (!strcmp(key, "session_header")) {
    if (!is_header_name(value)) return "session_header: solo letras, números y '-'";
    return copy(p->session_header, sizeof p->session_header, value) ? NULL : "session_header: demasiado largo";
  }
  return "clave desconocida";
}

int provider_insecure(const Provider *p) { return strncmp(p->url, "https://", 8) != 0; }

int provider_models_url(const Provider *p, char *out, size_t cap) {
  static const char tail[] = "/chat/completions";
  size_t n = strlen(p->url), t = sizeof tail - 1;
  const char *query = strchr(p->url, '?');
  size_t base = query ? (size_t)(query - p->url) : n;
  if (base < t || strncmp(p->url + base - t, tail, t)) return 0;
  if (base - t + strlen("/models") + (n - base) + 1 > cap) return 0;
  memcpy(out, p->url, base - t);
  strcpy(out + base - t, "/models");
  strcat(out, p->url + base); /* keep a query string such as ?api-version= */
  return 1;
}

int provider_parse_models(const char *json, char (*ids)[128], int max) {
  Json j = {0};
  int data, i, n = 0;
  if (!json_parse(&j, json)) return 0;
  data = json_key(&j, 0, "data");
  for (i = 0; i < json_size(&j, data) && n < max; i++) {
    char *id = json_str(&j, json_key(&j, json_index(&j, data, i), "id"));
    Provider probe;
    provider_defaults(&probe);
    if (*id && strlen(id) < 128 && !provider_set(&probe, "model", id)) strcpy(ids[n++], id);
    free(id);
  }
  json_free(&j);
  return n;
}
