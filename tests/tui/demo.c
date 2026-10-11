/* A scripted TinyShell screen for tests/tui/run.py: the real TUI (src/app/tui*.c) on the
 * real console adapter, with no network. Not part of the product. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "console.h"
#include "tui.h"

int main(void) {
  char line[256], key[64];
  const char *models[] = {"deepseek-v4.1-flash", "kimi-k2.7-code", "glm-5"};
  int pick, r;
  console_init();
  cancel_init();
  tui_init();
  tui_note("TinyShell demo \xc2\xb7 opencode-go \xc2\xb7 /ayuda");
  r = tui_prompt(line, sizeof line, "/home/pepa \xc2\xb7 0 in \xc2\xb7 0 out \xc2\xb7 0%", "deepseek-v4.1-flash");
  if (r != LINE_OK) return 2;
  tui_user(line);
  tui_thinking(1);
  tui_thinking(0);
  tui_stream("Hola, \xc2\xbfqu\xc3\xa9 tal? ");
  tui_stream("Miro los archivos.");
  tui_stream_end();
  tui_proposal("ls -la", "ver qu\xc3\xa9 hay en la carpeta", NULL);
  say("  ejecutar? [s/N] ");
  r = read_line(line, sizeof line, 1);
  tui_output(r == LINE_OK && !strcmp(line, "s") ? "uno.txt\ndos.txt" : "(rechazado)");
  pick = tui_select("Eleg\xc3\xad un modelo:", models, 3, 0);
  tui_note(pick >= 0 ? models[pick] : "(ninguno)");
  say("API key: ");
  r = read_secret(key, sizeof key);
  say("clave de %d caracteres\n", r == LINE_OK ? (int)strlen(key) : -1);
  r = tui_prompt(line, sizeof line, "fin", "");
  say(r == LINE_EOF ? "EOF\n" : "linea\n");
  return 0;
}
