/* Single place that sets the POSIX API level; include it first in every POSIX adapter.
 * POSIX.1-2008 only (no GNU extensions): the static musl build must run on any distro. */
#ifndef TINYSHELL_POSIX_H
#define TINYSHELL_POSIX_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#endif
