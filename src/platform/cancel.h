/* Ctrl+C handling. Ctrl+C never kills TinyShell abruptly: it cancels the model request or
 * the running command in progress, makes a pending prompt return EOF (an approval counts
 * as "no"; an idle prompt ends the session cleanly) and stops the current turn. What to
 * cancel is registered privately by each platform's HTTP and shell adapters. */
#ifndef TINYSHELL_CANCEL_H
#define TINYSHELL_CANCEL_H

void cancel_init(void);
int cancel_requested(void);
void cancel_reset(void); /* at the start of each user message */
/* 1 (once) if a pending console read was ended by Ctrl+C; readers then report EOF. */
int cancel_take_input_interrupt(void);

#endif
