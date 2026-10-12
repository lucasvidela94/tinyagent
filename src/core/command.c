#include "command.h"

#include <stdio.h>

#include "config.h"
#include "i18n.h"
#include "util.h"

char *command_result(const char *text, size_t dropped, int timed_out, int cancelled, unsigned long code) {
  char tail[128];
  Buf res = {0};
  buf_str(&res, *text ? text : tr("(no output)"));
  if (dropped) {
    snprintf(tail, sizeof tail, tr("\n[output truncated: %lu bytes left out]"), (unsigned long)dropped);
    buf_str(&res, tail);
  }
  if (cancelled) snprintf(tail, sizeof tail, "%s", tr("\n[cancelled by the user with Ctrl+C]"));
  else if (timed_out) snprintf(tail, sizeof tail, tr("\n[timed out: stopped after %d s]"), CMD_TIMEOUT_MS / 1000);
  else snprintf(tail, sizeof tail, tr("\n[exit=%lu]"), code);
  buf_str(&res, tail);
  return buf_take(&res);
}
