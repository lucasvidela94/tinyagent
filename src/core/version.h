/* Release version strings ("X.Y.Z", optional leading "v"). Portable C. */
#ifndef TINYSHELL_VERSION_H
#define TINYSHELL_VERSION_H

#include <stddef.h>

#define VERSION_INVALID (-2)

/* -1, 0 or 1 like strcmp, or VERSION_INVALID if either side is not X.Y.Z. */
int version_compare(const char *a, const char *b);

/* Extracts X.Y.Z from the output of `tinyshell --version` ("tinyshell X.Y.Z").
 * Returns 1 and fills out, or 0 if the output does not start that way. */
int version_from_output(const char *output, char *out, size_t cap);

#endif
