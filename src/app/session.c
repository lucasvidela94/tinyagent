#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "config.h"
#include "console.h"
#include "http.h"
#include "instructions.h"
#include "json.h"
#include "policy.h"
#include "machine.h"
#include "session.h"
#include "shell.h"
#include "stream.h"
#include "sys.h"
#include "telemetry.h"
#include "tui.h"
#include "util.h"

/* "tinyshell-" + 32 random hex digits. */
static void new_session_id(Session *s) {
  unsigned char r[16];
  int i;
  sys_random(r, sizeof r);
  strcpy(s->session_id, "tinyshell-");
  for (i = 0; i < 16; i++) snprintf(s->session_id + 10 + 2 * i, 3, "%02x", r[i]);
}

/* Instructions for this platform's shell, then the machine as detected at startup. */
static char *system_prompt(void) {
  char *head = instructions(system_dialect());
  Buf b = {0};
  buf_str(&b, head);
  free(head);
  buf_str(&b, "Environment (detected at startup): ");
  system_details(&b);
  return buf_take(&b);
}

void session_init(Session *s) {
  char *sys = system_prompt();
  new_session_id(s);
  memset(&s->history, 0, sizeof s->history);
  s->turns = s->proposed = s->approved = s->rejected = 0;
  s->started_ms = sys_ms();
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
  wipe(s->api_key, sizeof s->api_key);
}

/* The approval gate. Returns the text sent back to the model as the tool result. */
static char *decide(Session *s, const ToolCall *tc) {
  const char *problem = command_problem(tc), *risk;
  char answer[64], *out;
  if (problem) {
    say("\n");
    tui_error(problem);
    return xstrdup(problem);
  }
  risk = risk_reason(system_dialect(), tc->command);
  tui_proposal(tc->command, tc->reason, risk);
  s->proposed++;
  console_flush_input(); /* leftover pasted lines must not answer the question */
  say("  ejecutar? [s/N] ");
  if (read_line(answer, sizeof answer, 1) != LINE_OK || !is_yes(answer)) {
    s->rejected++;
    return xstrdup("Usuario rechazó la ejecución. No se ejecutó nada.");
  }
  if (risk) {
    console_flush_input();
    console_write("  escribí EJECUTAR para confirmar: ", COLOR_DANGER);
    if (read_line(answer, sizeof answer, 1) != LINE_OK || strcmp(answer, "EJECUTAR")) {
      s->rejected++;
      return xstrdup("Cancelado por seguridad. No se ejecutó nada.");
    }
  }
  s->approved++;
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

/* Handles one parsed response (text already shown if `streamed`). Returns 1 if the model
 * is waiting for tool results. */
static int handle_response(Session *s, const Json *j, int streamed) {
  int choice = json_index(j, json_key(j, 0, "choices"), 0);
  int msg = json_key(j, choice, "message"), calls = json_key(j, msg, "tool_calls"), i;
  char *text, *finish;
  count_usage(s, j);
  if (msg < 0) {
    char *e = json_str(j, json_key(j, json_key(j, 0, "error"), "message"));
    tui_error(*e ? e : "Respuesta sin message.");
    telemetry_error("api", "sin message", "chat");
    free(e);
    return 0;
  }
  text = json_str(j, json_key(j, msg, "content"));
  if (json_size(j, calls) > 0) {
    if (*text && !streamed) tui_text(text);
    history_add(&s->history, json_raw(j, msg), 0);
    for (i = 0; i < json_size(j, calls); i++) {
      ToolCall tc;
      char *out;
      tool_call_parse(j, json_index(j, calls, i), &tc);
      /* After Ctrl+C every remaining call still needs a result, but nothing is asked or run. */
      out = cancel_requested() ? xstrdup("Cancelado por el usuario con Ctrl+C. No se ejecutó nada.") : decide(s, &tc);
      history_add(&s->history, tool_result_json(tc.id, out), 0); /* every call gets a result */
      free(out);
      tool_call_free(&tc);
    }
    free(text);
    return !cancel_requested(); /* a cancelled turn stops here */
  }
  if (!streamed || !*text) tui_text(*text ? text : "(respuesta vacía)");
  finish = json_str(j, json_key(j, choice, "finish_reason"));
  if (!strcmp(finish, "length")) tui_note("(respuesta cortada por límite de tokens)");
  free(finish);
  history_add(&s->history, message_json("assistant", text), 0);
  free(text);
  return 0;
}

typedef struct { Stream st; int shown; } Live;

static void live_text(void *ctx, const char *text) {
  Live *l = (Live *)ctx;
  if (!l->shown) tui_thinking(0); /* the first words replace "pensando..." */
  l->shown = 1;
  tui_stream(text);
}

static void live_chunk(void *ctx, const char *data, size_t n) { stream_feed(&((Live *)ctx)->st, data, n, live_text, ctx); }

/* One streamed round trip: shows text as it arrives, returns the rebuilt response. */
static char *round_trip(Session *s, const char *body, int *shown) {
  Live l;
  char *resp = NULL;
  stream_init(&l.st);
  l.shown = 0;
  tui_thinking(1);
  if (http_post_stream(&s->provider, body, s->api_key, s->session_id, live_chunk, &l)) resp = stream_response(&l.st);
  if (!l.shown) tui_thinking(0);
  tui_stream_end();
  stream_free(&l.st);
  *shown = l.shown;
  return resp;
}

unsigned long session_send(Session *s, const char *text) {
  int turn;
  cancel_reset();
  history_add(&s->history, message_json("user", text), 1);
  for (turn = 0; turn < MAX_TURNS; turn++) {
    Json j = {0};
    int more = 0, shown = 0;
    char *body, *resp;
    history_trim(&s->history, MAX_HISTORY);
    body = build_request(&s->history, s->provider.model, system_dialect());
    resp = round_trip(s, body, &shown);
    free(body);
    s->turns++;
    if (!resp && cancel_requested()) {
      tui_note("cancelado");
      return 0;
    }
    if (!resp) {
      char code[32];
      unsigned long status = http_last_status();
      snprintf(code, sizeof code, status ? "http %lu" : "winhttp %lu", status ? status : http_last_error());
      telemetry_error("red", code, "chat");
      return status ? status : 1;
    }
    if (json_parse(&j, resp)) more = handle_response(s, &j, shown);
    else {
      tui_error("Respuesta JSON inválida. No se ejecutó nada.");
      telemetry_error("api", "json inválido", "chat");
    }
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
