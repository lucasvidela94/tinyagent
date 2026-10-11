#include "provider.h"

#include <ctype.h>
#include <string.h>

void provider_defaults(Provider *p) {
  strcpy(p->name, "opencode-go");
  strcpy(p->url, "https://opencode.ai/zen/go/v1/chat/completions");
  strcpy(p->model, "deepseek-v4.1-flash");
  strcpy(p->session_header, "x-opencode-session"); /* OpenCode Go answers 400 without it */
}

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
