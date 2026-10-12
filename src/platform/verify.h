/* Release authenticity checks: Windows CryptoAPI (advapi32/crypt32, XP SP3 and later) or,
 * on Linux, Mbed TLS. */
#ifndef TINYSHELL_VERIFY_H
#define TINYSHELL_VERIFY_H

#include <stddef.h>

int release_key_present(void); /* 0 while src/release_key.h is still the placeholder */

/* 1 only if sig is a valid RSA PKCS#1 v1.5 / SHA-256 signature of data made with the
 * release signing key. sig is in OpenSSL byte order (`openssl dgst -sha256 -sign`). */
int signature_valid(const unsigned char *data, size_t n, const unsigned char *sig, size_t siglen);

/* Lowercase hex SHA-256 of data into out (65 bytes). "" on failure. */
void sha256_hex(const unsigned char *data, size_t n, char out[65]);

#endif
