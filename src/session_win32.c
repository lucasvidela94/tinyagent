#include "win32.h"

#include <wincrypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "console.h"
#include "http.h"
#include "json.h"
#include "policy.h"
#include "prompt.h"
#include "session.h"
#include "shell.h"
#include "tui.h"
#include "util.h"

/* "tinyshell-" + 32 random hex digits from the CryptoAPI; tick count as a last resort. */
static void new_session_id(Session *s) {
  HCRYPTPROV prov = 0;
  BYTE r[16];
  int i, ok = CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
              CryptGenRandom(prov, sizeof r, r);
  if (prov) CryptReleaseContext(prov, 0);
  if (!ok)
    for (i = 0; i < 16; i++) r[i] = (BYTE)(GetTickCount() >> (i % 4 * 8)) ^ (BYTE)(i * 37);
  strcpy(s->session_id, "tinyshell-");
  for (i = 0; i < 16; i++) snprintf(s->session_id + 10 + 2 * i, 3, "%02x", r[i]);
}

void session_init(Session *s) {
  char *sys = system_prompt();
  new_session_id(s);
  memset(&s->history, 0, sizeof s->history);
  s->tokens_in = s->tokens_out = 0;
  history_add(&s->history, message_json("system", sys), 0);
  free(sys);
}

void session_reset(Session *s) {
  history_reset(&s->history);
  s->tokens_in = s->tokens_out = 0;
  new_session_id(s);
}

void session_free(Session *s) {
  history_free(&s->history);
  SecureZeroMemory(s->api_key, sizeof s->api_key);
}

/* The approval gate. Returns the text sent back to the model as the tool result. */
static char *decide(const ToolCall *tc) {
  const char *problem = command_problem(tc), *risk;
  char answer[64], *out;
  if (problem) {
    say("\n");
    tui_error(problem);
    return xstrdup(problem);
  }
  risk = risk_reason(tc->command);
  tui_proposal(tc->command, tc->reason, risk);
  console_flush_input(); /* leftover pasted lines must not answer the question */
  say("  ejecutar? [s/N] ");
  if (read_line(answer, sizeof answer, 1) != LINE_OK || !is_yes(answer))
    return xstrdup("Usuario rechazó la ejecución. No se ejecutó nada.");
  if (risk) {
    console_flush_input();
    console_write("  escribí EJECUTAR para confirmar: ", COLOR_DANGER);
    if (read_line(answer, sizeof answer, 1) != LINE_OK || strcmp(answer, "EJECUTAR"))
      return xstrdup("Cancelado por seguridad. No se ejecutó nada.");
  }
  out = run_command(tc->command);
  tui_output(out);
  return out;
}

static void count_usage(Session *s, const Json *j) {
  int usage = json_key(j, 0, "usage");
  char *in = json_str(j, json_key(j, usage, "prompt_tokens"));
  char *out = json_str(j, json_key(j, usage, "completion_tokens"));
  s->tokens_in += atol(in);
  s->tokens_out += atol(out);
  free(in);
  free(out);
}

/* Handles one parsed response. Returns 1 if the model is waiting for tool results. */
static int handle_response(Session *s, const Json *j) {
  int choice = json_index(j, json_key(j, 0, "choices"), 0);
  int msg = json_key(j, choice, "message"), calls = json_key(j, msg, "tool_calls"), i;
  char *text, *finish;
  count_usage(s, j);
  if (msg < 0) {
    char *e = json_str(j, json_key(j, json_key(j, 0, "error"), "message"));
    tui_error(*e ? e : "Respuesta sin message.");
    free(e);
    return 0;
  }
  text = json_str(j, json_key(j, msg, "content"));
  if (json_size(j, calls) > 0) {
    if (*text) tui_text(text);
    history_add(&s->history, json_raw(j, msg), 0);
    for (i = 0; i < json_size(j, calls); i++) {
      ToolCall tc;
      char *out;
      tool_call_parse(j, json_index(j, calls, i), &tc);
      out = decide(&tc);
      history_add(&s->history, tool_result_json(tc.id, out), 0); /* every call gets a result */
      free(out);
      tool_call_free(&tc);
    }
    free(text);
    return 1;
  }
  tui_text(*text ? text : "(respuesta vacía)");
  finish = json_str(j, json_key(j, choice, "finish_reason"));
  if (!strcmp(finish, "length")) tui_note("(respuesta cortada por límite de tokens)");
  free(finish);
  history_add(&s->history, message_json("assistant", text), 0);
  free(text);
  return 0;
}

unsigned long session_send(Session *s, const char *text) {
  int turn;
  history_add(&s->history, message_json("user", text), 1);
  for (turn = 0; turn < MAX_TURNS; turn++) {
    Json j = {0};
    int more = 0;
    char *body, *resp;
    history_trim(&s->history, MAX_HISTORY);
    body = build_request(&s->history);
    tui_thinking(1);
    resp = http_post(body, s->api_key, s->session_id);
    tui_thinking(0);
    free(body);
    if (!resp) return http_last_status() ? http_last_status() : 1;
    if (json_parse(&j, resp)) more = handle_response(s, &j);
    else tui_error("Respuesta JSON inválida. No se ejecutó nada.");
    json_free(&j);
    free(resp);
    if (!more) return 0;
  }
  {
    char msg[96];
    snprintf(msg, sizeof msg, "Límite de %d pasos para este mensaje. Escribí \"seguí\" para continuar.", MAX_TURNS);
    tui_note(msg);
  }
  return 0;
}
