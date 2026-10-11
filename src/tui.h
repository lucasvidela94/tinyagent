/* Minimal inline TUI in the spirit of Pi / OpenCode / Claude Code (Win32 adapter).
 * The conversation scrolls in the normal console buffer; only the editor (between two
 * rules) and a one-line footer are redrawn. Uses the console API, not ANSI escapes
 * (the Windows 7 console has none), and only glyphs present in the default raster font.
 * Falls back to plain lines when stdin/stdout are redirected or the console is narrow. */
#ifndef TINYSHELL_TUI_H
#define TINYSHELL_TUI_H

#include <stddef.h>

void tui_init(void);

/* Draws rule, editor, rule and footer ("left ... right"), reads one line in the editor
 * with the console's native line editing (arrows, F7 history, right-click paste), then
 * erases them; the caller echoes the message with tui_user(). Returns a LINE_* value. */
int tui_prompt(char *out, size_t cap, const char *left, const char *right);
void tui_user(const char *text); /* the submitted message, as a full-width shaded block */

void tui_thinking(int on); /* "pensando..." while waiting; erased when the answer arrives */
void tui_text(const char *text);  /* assistant text */
void tui_note(const char *text);  /* secondary information, dimmed */
void tui_error(const char *text); /* problems, in red */

/* A proposed command: "$ cmd" (yellow, or red when risky), the reason and the risk. */
void tui_proposal(const char *cmd, const char *reason, const char *risk);
/* Command output: first lines dimmed, then "... +N líneas · exit X". */
void tui_output(const char *output);

#endif
