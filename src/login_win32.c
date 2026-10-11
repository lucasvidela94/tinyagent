#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "credential.h"
#include "login.h"
#include "settings.h"
#include "tui.h"

#define MAX_PRESETS 16
#define MAX_MODELS 300

int login_pick_model(Provider *p, const char *key) {
  char url[600], (*ids)[128], *body, msg[200];
  const char **items;
  size_t len = 0;
  int n, i, current = 0, pick;
  if (!provider_models_url(p, url, sizeof url)) {
    tui_note("este proveedor no publica su lista de modelos; usá /modelo <nombre>");
    return 0;
  }
  if (!(body = http_get(url, key, MAX_HTTP, &len))) return 0;
  ids = (char(*)[128])malloc(MAX_MODELS * sizeof *ids);
  items = (const char **)malloc(MAX_MODELS * sizeof *items);
  n = ids && items ? provider_parse_models(body, ids, MAX_MODELS) : 0;
  free(body);
  if (!n) tui_error("la lista de modelos vino vacía o en un formato desconocido");
  for (i = 0; i < n; i++) {
    items[i] = ids[i];
    if (!strcmp(ids[i], p->model)) current = i;
  }
  snprintf(msg, sizeof msg, "Elegí un modelo de %s (%d; escribí para filtrar, Enter):", p->name, n);
  pick = n ? tui_select(msg, items, n, current) : -1;
  if (pick >= 0) {
    provider_set(p, "model", ids[pick]);
    tui_line("modelo: ", p->model, COLOR_ACCENT);
  }
  free(ids);
  free(items);
  return pick >= 0;
}

int login_run(Provider *p, char *key, size_t cap) {
  const char *items[MAX_PRESETS];
  char labels[MAX_PRESETS][160], secret[1024], msg[256];
  int i, n = provider_preset_count(), pick, ok;
  Provider chosen;
  if (n > MAX_PRESETS) n = MAX_PRESETS;
  for (i = 0; i < n; i++) {
    Provider q;
    provider_preset(i, &q);
    snprintf(labels[i], sizeof labels[i], "%-14s %s", q.name, provider_preset_label(i));
    items[i] = labels[i];
  }
  if ((pick = tui_select("Elegí un proveedor (flechas y Enter):", items, n, 0)) < 0) {
    tui_note("cancelado");
    return 0;
  }
  provider_preset(pick, &chosen);
  tui_line("proveedor: ", chosen.name, COLOR_ACCENT);
  say("\nAPI key de %s (clic derecho para pegar): ", chosen.name);
  if (read_secret(secret, sizeof secret) != LINE_OK || !*secret) {
    tui_note("cancelado: no se ingresó ninguna clave");
    SecureZeroMemory(secret, sizeof secret);
    return 0;
  }
  /* Fetching the model list doubles as a key check: a rejected key is never saved. */
  say("\n");
  if (!login_pick_model(&chosen, secret)) {
    unsigned long status = http_last_status();
    if (status == 401 || status == 403) {
      tui_error("la API key fue rechazada; no se guardó nada");
      SecureZeroMemory(secret, sizeof secret);
      return 0;
    }
    tui_note("se mantiene el modelo por defecto (/modelo para cambiarlo)");
  }
  ok = credential_save(chosen.name, secret);
  if (ok) {
    snprintf(msg, sizeof msg, "clave de %s guardada en el Administrador de credenciales de Windows", chosen.name);
    tui_note(msg);
    if (!settings_save_provider(&chosen)) tui_error("no se pudo escribir config.ini");
    snprintf(msg, sizeof msg, "listo · %s · %s", chosen.name, chosen.model);
    tui_note(msg);
    if (p) *p = chosen;
    if (key && strlen(secret) < cap) strcpy(key, secret);
  } else {
    tui_error("no se pudo guardar la clave en el Administrador de credenciales");
  }
  SecureZeroMemory(secret, sizeof secret);
  return ok;
}
