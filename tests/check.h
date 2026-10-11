/* Minimal test harness shared by the host-side test programs. */
#ifndef TINYSHELL_CHECK_H
#define TINYSHELL_CHECK_H

#include <stdio.h>

static int failures, checks;
#define CHECK(cond)                                           \
  do {                                                        \
    checks++;                                                 \
    if (!(cond)) {                                            \
      failures++;                                             \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
    }                                                         \
  } while (0)
#define CHECK_DONE(name) (printf("%s: %d checks, %d failures\n", name, checks, failures), failures != 0)

#endif
