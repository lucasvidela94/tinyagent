/* Release version strings ("X.Y.Z", optional leading "v") and the updater's file names.
 * Portable C. */
#ifndef TINYSHELL_VERSION_H
#define TINYSHELL_VERSION_H

#include <stddef.h>

#include "util.h"

#define VERSION_INVALID (-2)

/* -1, 0 or 1 like strcmp, or VERSION_INVALID if either side is not X.Y.Z. */
int version_compare(const char *a, const char *b);

/* Extracts X.Y.Z from the output of `tinyshell --version` ("tinyshell X.Y.Z").
 * Returns 1 and fills out, or 0 if the output does not start that way. */
int version_from_output(const char *output, char *out, size_t cap);

/* The updater's files next to the running binary: tag "new" or "old" goes before a final
 * ".exe" (any case), so Windows can still run it, or is appended when there is none:
 * C:\x\tinyshell.exe -> C:\x\tinyshell.new.exe, /usr/bin/tinyshell -> /usr/bin/tinyshell.new.
 * Same folder on purpose: renames stay on one volume. Appends to out. */
void version_sibling(const char *self, const char *tag, Buf *out);

#endif
