/* What Ctrl+C must stop, registered by the other Win32 adapters (private to win32/).
 * The console handler runs on its own thread, so handles are guarded: whoever releases
 * a handle first closes it, never both. */
#ifndef TINYSHELL_CANCEL_WIN32_H
#define TINYSHELL_CANCEL_WIN32_H

#include "win32.h"

#include <winhttp.h>

/* A WinHTTP request. release returns 1 if the caller still owns it and must close it,
 * 0 if Ctrl+C already closed it. */
void cancel_watch_http(HINTERNET request);
int cancel_release_http(HINTERNET request);

/* A running command: the job (may be NULL) and the process to terminate on Ctrl+C. */
void cancel_watch_process(HANDLE job, HANDLE process);
void cancel_release_process(void);

#endif
