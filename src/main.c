/* TinyShell: general-purpose terminal agent for old Windows PCs. CLI, approval gate and
 * the model round-trip loop.
 * The remote model only proposes commands; every command needs local approval. */
#include "win32.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "conversation.h"
#include "http.h"
#include "json.h"
#include "policy.h"
#include "prompt.h"
#include "shell.h"
#include "update.h"
#include "util.h"

/* The approval gate. Returns the text sent back to the model as the tool result. */
static char *decide(const ToolCall *tc) {
  const char *problem = command_problem(tc), *risk;
  char answer[64], *out;
  if (problem) {
    say("\n[RECHAZADO] %s\n", problem);
    return xstrdup(problem);
  }
  say("\n[PROPUESTA] %s\nMotivo: %s\n", tc->command, *tc->reason ? tc->reason : "(sin motivo)");
  risk = risk_reason(tc->command);
  if (risk) say("Riesgo: %s.\n", risk);
  say("Ejecutar? [s/N]: ");
  if (read_line(answer, sizeof answer, 1) != LINE_OK || !is_yes(answer))
    return xstrdup("Usuario rechazó la ejecución. No se ejecutó nada.");
  if (risk) {
    say("Comando potencialmente destructivo. Confirmá escribiendo EXACTAMENTE EJECUTAR: ");
    if (read_line(answer, sizeof answer, 1) != LINE_OK || strcmp(answer, "EJECUTAR"))
      return xstrdup("Cancelado por seguridad. No se ejecutó nada.");
  }
  say("Ejecutando (máximo %d s)...\n", CMD_TIMEOUT_MS / 1000);
  out = run_command(tc->command);
  say("%s\n", out);
  return out;
}

/* Handles one parsed response. Returns 1 if the model is waiting for tool results. */
static int handle_response(History *h, const Json *j) {
  int choice = json_index(j, json_key(j, 0, "choices"), 0);
  int msg = json_key(j, choice, "message"), calls = json_key(j, msg, "tool_calls"), i;
  char *text;
  if (msg < 0) {
    char *e = json_str(j, json_key(j, json_key(j, 0, "error"), "message"));
    say("\nRespuesta sin message%s%s\n", *e ? ": " : ".", e);
    free(e);
    return 0;
  }
  text = json_str(j, json_key(j, msg, "content"));
  if (json_size(j, calls) > 0) {
    if (*text) say("\n%s\n", text);
    history_add(h, json_raw(j, msg), 0);
    for (i = 0; i < json_size(j, calls); i++) {
      ToolCall tc;
      char *out;
      tool_call_parse(j, json_index(j, calls, i), &tc);
      out = decide(&tc);
      history_add(h, tool_result_json(tc.id, out), 0); /* every call gets a result, even refused */
      free(out);
      tool_call_free(&tc);
    }
    free(text);
    return 1;
  }
  say("\n%s\n", *text ? text : "(respuesta vacía)");
  {
    char *finish = json_str(j, json_key(j, choice, "finish_reason"));
    if (!strcmp(finish, "length")) say("(respuesta cortada por límite de tokens)\n");
    free(finish);
  }
  history_add(h, message_json("assistant", text), 0);
  free(text);
  return 0;
}

/* Model round trips for the latest user message, until a text answer or MAX_TURNS. */
static void converse(History *h, const char *api_key) {
  int turn;
  for (turn = 0; turn < MAX_TURNS; turn++) {
    Json j = {0};
    int more = 0;
    char *body, *resp;
    history_trim(h, MAX_HISTORY);
    body = build_request(h);
    resp = http_post(body, api_key);
    free(body);
    if (!resp) return;
    if (json_parse(&j, resp)) more = handle_response(h, &j);
    else say("\nRespuesta JSON inválida. No se ejecutó nada.\n");
    json_free(&j);
    free(resp);
    if (!more) return;
  }
  say("\nLímite de %d pasos por mensaje. Escribí \"seguí\" para continuar.\n", MAX_TURNS);
}

static int usage(void) {
  say("Uso: tinyshell [--version | --update | --rollback | --help]\n"
      "  sin argumentos  inicia el agente (pide la API key de OpenCode Go)\n"
      "  --version       muestra la versión\n"
      "  --update        busca, verifica e instala la última versión\n"
      "  --rollback      vuelve a la versión anterior (tinyshell.old.exe)\n");
  return 0;
}

static int run_flag(const char *flag) {
  if (!strcmp(flag, "--version") || !strcmp(flag, "-v")) {
    say("tinyshell %s\n", TINYSHELL_VERSION);
    return 0;
  }
  if (!strcmp(flag, "--update")) return update_run();
  if (!strcmp(flag, "--rollback")) return update_rollback();
  if (!strcmp(flag, "--help") || !strcmp(flag, "-h") || !strcmp(flag, "/?")) return usage();
  say("Opción desconocida: %s\n", flag);
  usage();
  return 2;
}

int main(int argc, char **argv) {
  char api[1024], input[MAX_LINE];
  History h = {0};
  int r;
  console_init();
  if (argc > 1) return run_flag(argv[1]);
  say("TinyShell %s | OpenCode Go / %s\n", TINYSHELL_VERSION, MODEL);
  say("El modelo es remoto; esta PC solo ejecuta comandos que vos aprobás.\n");
  say("Comandos: /salir, /reiniciar (borra el historial), /actualizar.\n\nAPI key de OpenCode Go (no se muestra): ");
  if (read_line(api, sizeof api, 0) != LINE_OK || !*api) {
    say("Falta API key.\n");
    return 1;
  }
  {
    char *sys = system_prompt();
    history_add(&h, message_json("system", sys), 0);
    free(sys);
  }
  for (;;) {
    say("\nvos> ");
    r = read_line(input, sizeof input, 1);
    if (r == LINE_EOF) break;
    if (r == LINE_TOO_LONG) {
      say("Mensaje demasiado largo (máximo %d bytes). No se envió.\n", MAX_LINE - 1);
      continue;
    }
    if (!strcmp(input, "/salir") || !strcmp(input, "/exit")) break;
    if (!strcmp(input, "/actualizar")) {
      update_run();
      continue;
    }
    if (!strcmp(input, "/reiniciar")) {
      history_reset(&h);
      say("Historial borrado.\n");
      continue;
    }
    if (!*input) continue;
    history_add(&h, message_json("user", input), 1);
    converse(&h, api);
  }
  SecureZeroMemory(api, sizeof api);
  history_free(&h);
  say("\nSaliendo.\n");
  return 0;
}
