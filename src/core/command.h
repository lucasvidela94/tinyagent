/* The text a command result goes back to the model as. Portable C, shared by every
 * platform's shell adapter so the model sees the same format everywhere. */
#ifndef TINYSHELL_COMMAND_H
#define TINYSHELL_COMMAND_H

#include <stddef.h>

/* text: the command's output as UTF-8 (NUL bytes already replaced). Returns it, or
 * "(no output)", followed by a truncation note if `dropped` bytes were cut, and then either
 * the Ctrl+C note, the timeout note or "[exit=N]": trailing "[...]" lines, in the interface
 * language (tr()). Caller frees. */
char *command_result(const char *text, size_t dropped, int timed_out, int cancelled, unsigned long code);

#endif
