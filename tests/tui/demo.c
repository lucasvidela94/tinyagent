/* A scripted TinyShell screen for tests/tui/run.py: the real TUI (src/app/tui*.c) on the
 * real console adapter, with no network. Not part of the product. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cancel.h"
#include "console.h"
#include "question.h"
#include "tui.h"

int main(void) {
  char line[256], key[64];
  const char *models[] = {"deepseek-v4.1-flash", "kimi-k2.7-code", "glm-5"};
  int pick, r;
  console_init();
  cancel_init();
  tui_init();
  /* Interface labels are English, as in the source; the user's and the model's words are
   * Spanish, to check UTF-8 and the ASCII fallback. */
  tui_note("TinyShell demo \xc2\xb7 opencode-go \xc2\xb7 /help");
  r = tui_prompt(line, sizeof line, "/home/pepa \xc2\xb7 0 in \xc2\xb7 0 out \xc2\xb7 0%", "deepseek-v4.1-flash");
  if (r != LINE_OK) return 2;
  tui_user(line);
  tui_thinking(1);
  tui_thinking(0);
  tui_stream("Hola, \xc2\xbfqu\xc3\xa9 tal? ");
  tui_stream("Miro los archivos.");
  tui_stream_end();
  tui_panel("$ ls -la", "ver qu\xc3\xa9 hay en la carpeta");
  {
    const char *answers[] = {"Yes, once", "Yes, always \"ls\" in this session", "No"};
    pick = tui_select("Run it?", answers, 3, 0);
  }
  if (pick == 0)
    tui_output("l1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nreadme.txt\nsetup.log\n[exit=0]");
  else
    tui_panel_end("(rejected)");
  pick = tui_select("Pick a model:", models, 3, 0);
  tui_note(pick >= 0 ? models[pick] : "(none)");
  say("API key: ");
  r = read_secret(key, sizeof key);
  say("key of %d bytes\n", r == LINE_OK ? (int)strlen(key) : -1);
  {
    ToolCall tc;
    char *answer;
    memset(&tc, 0, sizeof tc);
    tc.question = (char *)"\xc2\xbfQu\xc3\xa9 disco reviso?";
    tc.options[0] = (char *)"C:";
    tc.options[1] = (char *)"D:";
    tc.noptions = 2;
    answer = question_ask(&tc);
    say("[%s]\n", answer);
    free(answer);
  }
  r = tui_prompt(line, sizeof line, "end", "");
  say(r == LINE_EOF ? "EOF\n" : "line\n");
  return 0;
}
