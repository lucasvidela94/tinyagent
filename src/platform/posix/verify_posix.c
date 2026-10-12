/* Release authenticity checks with Mbed TLS (the same library the HTTPS client uses). */
#include "posix.h"

#include <stdio.h>
#include <stdlib.h>

#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <psa/crypto.h>

#include "release_key.h"
#include "verify.h"

int release_key_present(void) { return RELEASE_PUBLIC_KEY_LEN > 0; }

static int sha256(const unsigned char *data, size_t n, unsigned char digest[32]) {
  size_t len = 0;
  return psa_crypto_init() == PSA_SUCCESS &&
         psa_hash_compute(PSA_ALG_SHA_256, data, n, digest, 32, &len) == PSA_SUCCESS && len == 32;
}

/* sig is in OpenSSL order (big-endian), which is what Mbed TLS takes; no reversal as on Windows. */
int signature_valid(const unsigned char *data, size_t n, const unsigned char *sig, size_t siglen) {
  mbedtls_pk_context pk;
  unsigned char digest[32];
  int ok;
  if (!release_key_present() || siglen == 0 || siglen > 2048 || !sha256(data, n, digest)) return 0;
  mbedtls_pk_init(&pk);
  ok = mbedtls_pk_parse_public_key(&pk, RELEASE_PUBLIC_KEY, RELEASE_PUBLIC_KEY_LEN) == 0 &&
       mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, digest, sizeof digest, sig, siglen) == 0;
  mbedtls_pk_free(&pk);
  return ok;
}

void sha256_hex(const unsigned char *data, size_t n, char out[65]) {
  unsigned char digest[32];
  int i;
  out[0] = 0;
  if (!sha256(data, n, digest)) return;
  for (i = 0; i < 32; i++) snprintf(out + 2 * i, 3, "%02x", digest[i]);
}
