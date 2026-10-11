/* Streaming chat completions (server-sent events). Portable C.
 * Feed the response bytes as they arrive; text deltas are reported at once, tool-call
 * pieces are joined, and at the end the equivalent non-streaming response is rebuilt so
 * the rest of the program handles both the same way. A server that ignores "stream"
 * and answers with plain JSON is handled too: stream_response returns that body. */
#ifndef TINYSHELL_STREAM_H
#define TINYSHELL_STREAM_H

#include "util.h"

#define STREAM_MAX_CALLS 16

typedef struct { Buf id, name, args; } StreamCall;

typedef struct {
  Buf line, raw, content;
  StreamCall calls[STREAM_MAX_CALLS];
  int ncalls, events, done;
  char finish[32];
  long prompt_tokens, completion_tokens;
} Stream;

typedef void (*StreamText)(void *ctx, const char *text);

void stream_init(Stream *s);
void stream_free(Stream *s);
void stream_feed(Stream *s, const char *data, size_t n, StreamText on_text, void *ctx);

/* {"choices":[{"message":{...},"finish_reason":..}],"usage":{...}} rebuilt from the events,
 * or a copy of the body if no event arrived (non-streaming server or an error body). */
char *stream_response(const Stream *s);

#endif
