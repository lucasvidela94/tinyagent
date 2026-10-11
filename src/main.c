/* TinyShell: general-purpose terminal agent for old Windows PCs. Entry point: CLI flags,
 * API key setup and the chat loop with its slash commands. */
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "credential.h"
#include "policy.h"
#include "session.h"
#include "settings.h"
#include "tui.h"
#include "update.h"
#include "util.h"

static int usage(void) {
  say("Uso: tinyshell [--version | --update | --rollback | --logout | --help]\n"
      "  sin argumentos  abre una sesión\n"
      "  --version       muestra la versión\n"
      "  --update        busca, verifica e instala la última versión\n"
      "  --rollback      vuelve a la versión anterior (tinyshell.old.exe)\n"
      "  --logout        borra la API key guardada del proveedor actual\n"
      "Proveedor y modelo: %s\n", settings_path());
  return 0;
}

static int run_flag(const char *flag) {
  Provider p;
  settings_load(&p);
  if (!strcmp(flag, "--version") || !strcmp(flag, "-v")) {
    say("tinyshell %s\n", TINYSHELL_VERSION);
    return 0;
  }
  if (!strcmp(flag, "--update")) return update_run();
  if (!strcmp(flag, "--rollback")) return update_rollback();
  if (!strcmp(flag, "--logout")) {
    say(credential_delete(p.name) ? "API key de %s borrada.\n" : "No se pudo borrar la API key de %s.\n", p.name);
    return 0;
  }
  if (!strcmp(flag, "--help") || !strcmp(flag, "-h") || !strcmp(flag, "/?")) return usage();
  say("Opción desconocida: %s\n", flag);
  usage();
  return 2;
}

/* Asks for the key (hidden) and offers to save it. Empty is allowed (local servers
 * usually need none). 0 only on EOF. */
static int ask_key(const Provider *p, char *key, size_t cap) {
  char answer[16], msg[96];
  say("API key de %s (no se muestra; clic derecho para pegar; Enter si no usa): ", p->name);
  if (read_line(key, cap, 0) != LINE_OK) return 0;
  if (!*key) {
    tui_note("sin API key");
    return 1;
  }
  snprintf(msg, sizeof msg, "clave recibida (%d caracteres)", (int)strlen(key));
  tui_note(msg);
  say("¿Guardarla para este usuario de Windows? [s/N] ");
  if (read_line(answer, sizeof answer, 1) == LINE_OK && is_yes(answer))
    tui_note(credential_save(p->name, key) ? "guardada · tinyshell --logout la borra" : "no se pudo guardar");
  return 1;
}

/* "/modelo" shows the provider; "/modelo X" switches model and saves it to config.ini. */
static void model_command(Session *s, const char *arg) {
  Buf msg = {0};
  const char *err;
  while (*arg == ' ') arg++;
  if (*arg) {
    if ((err = provider_set(&s->provider, "model", arg)) != NULL) {
      tui_error(err);
      return;
    }
    if (!settings_save_model(arg)) tui_error("no se pudo guardar en config.ini (vale para esta sesión)");
  }
  buf_str(&msg, s->provider.name);
  buf_str(&msg, " · ");
  buf_str(&msg, s->provider.model);
  buf_str(&msg, "\n");
  buf_str(&msg, s->provider.url);
  buf_str(&msg, "\nse cambia en ");
  buf_str(&msg, settings_path());
  tui_note(msg.p);
  free(msg.p);
}

static void help(void) {
  tui_note("/modelo [x]  ver el proveedor o cambiar de modelo\n"
           "/clave       cambiar la API key\n"
           "/reiniciar   empezar una conversación nueva\n"
           "/actualizar  buscar e instalar la última versión\n"
           "/salir       terminar");
}

static void fmt_tokens(char *out, size_t cap, long n) {
  if (n < 1000) snprintf(out, cap, "%ld", n);
  else snprintf(out, cap, "%ld.%ldk", n / 1000, (n % 1000) / 100);
}

static void status_left(const Session *s, char *out, size_t cap) {
  wchar_t wcwd[MAX_PATH];
  char cwd[MAX_PATH * 3] = "", in[16], o[16];
  int pct = (int)(s->history.bytes * 100 / MAX_HISTORY);
  if (GetCurrentDirectoryW(MAX_PATH, wcwd)) WideCharToMultiByte(CP_UTF8, 0, wcwd, -1, cwd, sizeof cwd, NULL, NULL);
  fmt_tokens(in, sizeof in, s->tokens_in);
  fmt_tokens(o, sizeof o, s->tokens_out);
  snprintf(out, cap, "%s · %s in · %s out · %d%%", cwd, in, o, pct > 100 ? 100 : pct);
}

static void chat(Session *s) {
  char input[MAX_LINE], left[MAX_PATH * 3 + 64];
  for (;;) {
    int r;
    unsigned long status;
    status_left(s, left, sizeof left);
    r = tui_prompt(input, sizeof input, left, s->provider.model);
    if (r == LINE_EOF || !strcmp(input, "/salir") || !strcmp(input, "/exit")) return;
    if (r == LINE_OK && *input) tui_user(input);
    if (r == LINE_TOO_LONG) tui_error("Mensaje demasiado largo; no se envió.");
    else if (!*input) continue;
    else if (!strcmp(input, "/ayuda") || !strcmp(input, "/help")) help();
    else if (!strcmp(input, "/reiniciar")) {
      session_reset(s);
      tui_note("conversación nueva");
    } else if (!strcmp(input, "/actualizar")) update_run();
    else if (!strcmp(input, "/clave")) ask_key(&s->provider, s->api_key, sizeof s->api_key);
    else if (!strncmp(input, "/modelo", 7) && (!input[7] || input[7] == ' ')) model_command(s, input + 7);
    else if (input[0] == '/') tui_error("Comando desconocido. /ayuda muestra los comandos.");
    else if ((status = session_send(s, input)) == 401 || status == 403)
      tui_error("La API key fue rechazada. /clave para cambiarla.");
  }
}

int main(int argc, char **argv) {
  Session s;
  char banner[160];
  console_init();
  if (argc > 1) return run_flag(argv[1]);
  tui_init();
  settings_load(&s.provider);
  snprintf(banner, sizeof banner, "TinyShell %s · %s · /ayuda", TINYSHELL_VERSION, s.provider.name);
  tui_note(banner);
  if (provider_insecure(&s.provider)) tui_error("aviso: la URL es http://, la API key y la conversación viajan sin cifrar");
  if (credential_load(s.provider.name, s.api_key, sizeof s.api_key))
    tui_note("usando la API key guardada · /clave para cambiarla");
  else if (!ask_key(&s.provider, s.api_key, sizeof s.api_key)) {
    tui_error("Falta la API key.");
    SecureZeroMemory(s.api_key, sizeof s.api_key);
    return 1;
  }
  session_init(&s);
  chat(&s);
  session_free(&s);
  return 0;
}
