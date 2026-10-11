/* TinyShell Agent for Windows 7 x86: CLI, approval gate and the model round-trip loop.
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
#include "shell.h"
#include "util.h"

static const char *SYSTEM_PROMPT =
    "Sos un agente técnico para diagnosticar y reparar Windows 7 de 32 bits, Atom N2600 y 2 GB RAM. "
    "Respondé en español, breve y práctico. Tenés la herramienta shell para ejecutar comandos CMD en la máquina real; "
    "cada comando lo aprueba una persona. Primero inspeccioná y diagnosticá; preferí comandos de lectura como "
    "systeminfo, tasklist, wmic, ipconfig, sc query, dir, type, chkdsk sin /f. Ejecutá una sola acción por vez y evitá "
    "encadenar con & o redirigir con >. Los comandos corren sin entrada interactiva y con límite de tiempo: no uses "
    "comandos que esperen respuesta. Para modificar, instalar, borrar, cambiar registro, servicios o configuración, "
    "explicá el riesgo, recomendá hacer copia de seguridad de los archivos importantes y pedí aprobación en el chat "
    "antes de llamar shell. Nunca borres datos, formatees discos ni desactives antivirus/firewall. No supongas que una "
    "reparación funcionó: verificá el resultado.";

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

int main(void) {
  char api[1024], input[MAX_LINE];
  History h = {0};
  int r;
  console_init();
  say("TinyShell Agent | Windows 7 x86 | OpenCode Go / %s\n", MODEL);
  say("El modelo es remoto; esta PC solo ejecuta comandos que vos aprobás.\n");
  say("Comandos: /salir, /reiniciar (borra el historial).\n\nAPI key de OpenCode Go (no se muestra): ");
  if (read_line(api, sizeof api, 0) != LINE_OK || !*api) {
    say("Falta API key.\n");
    return 1;
  }
  history_add(&h, message_json("system", SYSTEM_PROMPT), 0);
  for (;;) {
    say("\nvos> ");
    r = read_line(input, sizeof input, 1);
    if (r == LINE_EOF) break;
    if (r == LINE_TOO_LONG) {
      say("Mensaje demasiado largo (máximo %d bytes). No se envió.\n", MAX_LINE - 1);
      continue;
    }
    if (!strcmp(input, "/salir") || !strcmp(input, "/exit")) break;
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
