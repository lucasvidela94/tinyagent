#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "httpwire.h"

static void sink(void *ctx, const char *d, size_t n) {
  (void)ctx;
  (void)d;
  (void)n;
}

/* libFuzzer entry: whatever a server (or a man in the middle) sends, split at odd sizes. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  HttpwireResp r;
  size_t off = 0;
  int st = HTTPWIRE_MORE;
  httpwire_init(&r);
  while (off < size && st == HTTPWIRE_MORE) {
    size_t n = (off * 5 % 11) + 1;
    if (n > size - off) n = size - off;
    st = httpwire_feed(&r, (const char *)data + off, n, sink, NULL);
    off += n;
  }
  if (st == HTTPWIRE_MORE) httpwire_eof(&r);
  httpwire_free(&r);
  return 0;
}
