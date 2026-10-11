/* Single place that pulls in <windows.h> with the Windows 7 API level. */
#ifndef TINYSHELL_WIN32_H
#define TINYSHELL_WIN32_H

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 /* Windows 7: do not use newer APIs */
#endif
#include <windows.h>

#endif
