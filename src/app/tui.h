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
void tui_stream(const char *text); /* a streamed piece of assistant text, printed at once */
void tui_stream_end(void);         /* ends the streamed paragraph, if any */
void tui_note(const char *text);  /* secondary information, dimmed */
void tui_error(const char *text); /* problems, in red */

/* Menu (tui_select.c): arrows move, Enter picks, Esc cancels; menus of more than 5 items
 * filter as you type and show at most 10 rows at a time. Takes only the rows it needs and
 * erases itself and its title when done, so the caller shows the choice. Returns the
 * chosen index or -1. Without a console it prints a numbered list and reads a number or a
 * name (Enter = initial). */
int tui_select(const char *title, const char *const *items, int n, int initial);

void tui_line(const char *indent, const char *text, int color); /* one line in a color */
int tui_is_fancy(void);

/* A shaded panel, like OpenCode's tool blocks: `head` ("$ cmd", a question) and a dimmer
 * `detail` (the reason; may be NULL). It stays open for tui_output or tui_panel_end. */
void tui_panel(const char *head, const char *detail);
/* Closes the open panel, with a last dim line (an answer, "not run") if `text` is set. */
void tui_panel_end(const char *text);
/* Command output in the open panel: "(N earlier lines)", the last lines, then the exit
 * code and other notes; closes the panel. */
void tui_output(const char *output);

#endif
