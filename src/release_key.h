/* Public half of the release signing key (DER SubjectPublicKeyInfo).
 * Placeholder: no key configured yet, so updates are refused.
 * Generate the real one once with build/gen-signing-key.sh and commit this file. */
#ifndef TINYSHELL_RELEASE_KEY_H
#define TINYSHELL_RELEASE_KEY_H

static const unsigned char RELEASE_PUBLIC_KEY[1] = {0};
#define RELEASE_PUBLIC_KEY_LEN 0

#endif
