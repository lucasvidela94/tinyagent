/* Ctrl+C handling (Win32 adapter). Ctrl+C never kills TinyShell abruptly: it cancels the
 * model request or the running command in progress, makes a pending prompt return EOF
 * (an approval counts as "no"; an idle prompt ends the session cleanly) and stops the
 * current turn. The console handler runs on its own thread, so registered handles are
 * guarded: whoever releases a handle first closes it, never both. */
#ifndef TINYSHELL_CANCEL_H
#define TINYSHELL_CANCEL_H

void cancel_init(void);
int cancel_requested(void);
void cancel_reset(void); /* at the start of each user message */
/* 1 (once) if a pending console read was ended by Ctrl+C; readers then report EOF. */
int cancel_take_input_interrupt(void);

/* A WinHTTP request handle (void * to keep winhttp.h out of callers). release returns 1
 * if the caller still owns it and must close it, 0 if Ctrl+C already closed it. */
void cancel_watch_http(void *request);
int cancel_release_http(void *request);

/* A running command: the job (may be NULL) and the process to terminate on Ctrl+C. */
void cancel_watch_process(void *job, void *process);
void cancel_release_process(void);

#endif
