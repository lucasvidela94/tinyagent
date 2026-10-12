/* The machine TinyShell runs on, as told to the model and put in reports: OS, hardware and
 * which shell dialect applies. One adapter per platform. */
#ifndef TINYSHELL_MACHINE_H
#define TINYSHELL_MACHINE_H

#include "dialect.h"
#include "util.h"

void system_os(Buf *b);      /* "Windows 7 (6.1 build 7601, Service Pack 1)", "Debian GNU/Linux 7 (kernel 3.2.0)" */
void system_details(Buf *b); /* one line: OS, 32/64-bit, RAM, CPUs, current folder */
const Dialect *system_dialect(void); /* the shell run_command uses: risk rules and prompt guidance */
/* The user's interface language as a code i18n_parse() reads ("es", "en", "es_AR.UTF-8"). */
const char *system_language(void);

#endif
