/* Which chat-completions service and model to talk to. Portable C.
 * Any OpenAI-compatible endpoint works (OpenCode Go, OpenAI, OpenRouter, DeepSeek, a
 * LM Studio/Ollama server on the LAN...). No secrets here: API keys live in the
 * Credential Manager, one per provider name. */
#ifndef TINYSHELL_PROVIDER_H
#define TINYSHELL_PROVIDER_H

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

#endif
