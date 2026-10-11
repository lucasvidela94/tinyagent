/* `tinyshell reporte` / `/reporte` (Win32 adapter): the user describes the problem; the
 * report (system, provider, recent errors and, if accepted, this session's conversation)
 * is saved to %LOCALAPPDATA%\TinyShell\reportes\*.txt, shown, and sent only after "s".
 * The relay files a public GitHub issue with the redacted summary and keeps the full
 * report private. Without network, the user is told to email the saved file. */
#ifndef TINYSHELL_REPORTER_H
#define TINYSHELL_REPORTER_H

#include "session.h"

int reporter_run(const Session *s); /* s may be NULL (from the command line) */

#endif
