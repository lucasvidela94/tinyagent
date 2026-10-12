/* Mozilla's root certificates, embedded (generated: see ca_bundle.c). Private to posix/. */
#ifndef TINYSHELL_CA_BUNDLE_H
#define TINYSHELL_CA_BUNDLE_H

extern const char *const CA_BUNDLE[]; /* one PEM certificate per entry */
extern const int CA_BUNDLE_COUNT;

#endif
