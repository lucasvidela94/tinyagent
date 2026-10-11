#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stream.h"

/* libFuzzer entry: chunks SSE partidos en cualquier punto, incluso UTF-8. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 65536) return 0;
  Stream st;
  stream_init(&st);
  /* alimenta de a trozos impares para simular TCP fragmentado */
  size_t off = 0;
  while (off < size) {
    size_t n = (off * 7 % 13) + 1;
    if (n > size - off) n = size - off;
    if (n > 16) n = 16;
    stream_feed(&st, (const char *)(data + off), n, NULL, NULL);
    off += n;
  }
  char *out = stream_response(&st);
  free(out);
  stream_free(&st);
  return 0;
}
