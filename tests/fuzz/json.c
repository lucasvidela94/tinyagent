#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

/* libFuzzer entry: the JSON parser reads remote input; garbage must never crash it. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 65536) return 0;
  char *tmp = malloc(size + 1);
  if (!tmp) return 0;
  memcpy(tmp, data, size);
  tmp[size] = 0;
  Json j = {0};
  if (json_parse(&j, tmp)) {
    char *s = json_str(&j, json_key(&j, 0, "content"));
    free(s);
    json_free(&j);
  }
  free(tmp);
  return 0;
}
