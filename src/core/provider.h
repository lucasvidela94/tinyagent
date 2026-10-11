/* Which chat-completions service and model to talk to. Portable C.
 * Any OpenAI-compatible endpoint works (OpenCode Go, OpenAI, OpenRouter, DeepSeek, a
 * LM Studio/Ollama server on the LAN...). No secrets here: API keys live in the
 * Credential Manager, one per provider name. */
#ifndef TINYSHELL_PROVIDER_H
#define TINYSHELL_PROVIDER_H

#include <stddef.h>

typedef struct {
  char name[64];           /* also names the saved API key */
  char url[512];           /* full chat-completions URL, http:// or https:// */
  char model[128];
  char session_header[64]; /* per-conversation ID header ("" = none) */
} Provider;

void provider_defaults(Provider *p); /* OpenCode Go with deepseek-v4.1-flash */

/* Sets one config key (name, url, model, session_header). Returns NULL on success or a
 * short Spanish error, leaving the field unchanged. Unknown keys are an error. */
const char *provider_set(Provider *p, const char *key, const char *value);

int provider_insecure(const Provider *p); /* 1 if the API key would travel unencrypted */

/* Providers offered by `tinyshell login`. Adding one is adding a row in provider.c. */
int provider_preset_count(void);
const char *provider_preset_label(int i); /* e.g. "OpenCode Go (el modelo se elige después)" */
void provider_preset(int i, Provider *p);

/* <base>/models for a ".../chat/completions" URL (query string kept). 0 if the URL has
 * another shape, so the model list cannot be fetched. */
int provider_models_url(const Provider *p, char *out, size_t cap);
/* Model ids from an OpenAI-style {"data":[{"id":...}]} list, in server order. Invalid or
 * oversized ids are skipped. Returns how many were stored (0 on malformed JSON). */
int provider_parse_models(const char *json, char (*ids)[128], int max);

#endif
