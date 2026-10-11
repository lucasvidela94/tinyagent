#include "command.h"

#include <stdio.h>

#include "config.h"
#include "util.h"

char *command_result(const char *text, size_t dropped, int timed_out, int cancelled, unsigned long code) {
  char tail[96];
  Buf res = {0};
  buf_str(&res, *text ? text : "(sin salida)");
  if (dropped) {
    snprintf(tail, sizeof tail, "\n[salida truncada: %lu bytes omitidos]", (unsigned long)dropped);
    buf_str(&res, tail);
  }
  if (cancelled) snprintf(tail, sizeof tail, "\n[cancelado por el usuario con Ctrl+C]");
  else if (timed_out) snprintf(tail, sizeof tail, "\n[tiempo agotado: terminado tras %d s]", CMD_TIMEOUT_MS / 1000);
  else snprintf(tail, sizeof tail, "\n[exit=%lu]", code);
  buf_str(&res, tail);
  return buf_take(&res);
}
