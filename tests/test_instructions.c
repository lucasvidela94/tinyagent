/* Regression tests for src/core/instructions.c with both dialects: each platform's prompt
 * names its own shell and tools and none of the other's. Run: ./build/test-core.sh */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "config.h"
#include "conversation.h"
#include "instructions.h"
#include "json.h"

static int has(const char *s, const char *part) { return strstr(s, part) != NULL; }

int main(void) {
  char *win = instructions(&DIALECT_CMD), *lin = instructions(&DIALECT_SH), limits[64];
  snprintf(limits, sizeof limits, "stopped after %d seconds", CMD_TIMEOUT_MS / 1000);
  CHECK(has(win, "Windows PC") && has(win, "one cmd.exe command") && has(win, "batch scripts"));
  CHECK(has(win, "reg query") && has(win, "registry"));
  CHECK(!has(win, "/bin/sh") && !has(win, "Linux") && !has(win, "sudo"));
  CHECK(has(lin, "Linux PC") && has(lin, "one /bin/sh command") && has(lin, "shell scripts"));
  CHECK(has(lin, "busybox") && has(lin, "packages") && has(lin, "sudo"));
  CHECK(!has(lin, "cmd.exe") && !has(lin, "Windows") && !has(lin, "wmic") && !has(lin, "registry"));
  CHECK(has(win, limits) && has(lin, limits)); /* limits come from config.h */
  CHECK(has(win, "approves every command") && has(lin, "approves every command"));
  CHECK(has(win, "Tool: `question`") && has(lin, "Tool: `question`"));
  CHECK(has(win, "do not ask for agreement in chat") && !has(win, "wait for the user to agree")); /* one gate */
  free(win);
  free(lin);
  { /* the shell tool sent with every request describes the same shell */
    History h;
    memset(&h, 0, sizeof h);
    win = build_request(&h, "m", &DIALECT_CMD);
    lin = build_request(&h, "m", &DIALECT_SH);
    CHECK(has(win, "Run one cmd.exe command on the local Windows machine") && !has(win, "/bin/sh"));
    CHECK(has(lin, "Run one /bin/sh command on the local Linux machine") && !has(lin, "cmd.exe"));
    CHECK(has(win, "\"name\":\"question\"") && has(win, "\"maxItems\":8"));
    snprintf(limits, sizeof limits, "\"max_tokens\":%d,", MAX_ANSWER_TOKENS);
    CHECK(has(win, limits) && MAX_ANSWER_TOKENS >= 16384); /* room to think and still answer */
    {
      Json j = {0};
      CHECK(json_parse(&j, win)); /* both tools in one well-formed body */
      CHECK(json_size(&j, json_key(&j, 0, "tools")) == 2);
      json_free(&j);
    }
    free(win);
    free(lin);
  }
  return CHECK_DONE("test_instructions");
}
