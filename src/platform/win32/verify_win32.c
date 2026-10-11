#include "win32.h"

#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

#include "release_key.h"
#include "verify.h"

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#endif

int release_key_present(void) { return RELEASE_PUBLIC_KEY_LEN > 0; }

static int sha256(HCRYPTPROV prov, const unsigned char *data, size_t n, HCRYPTHASH *hash) {
  *hash = 0;
  if (n > 0xFFFFFFFFu) return 0;
  if (!CryptCreateHash(prov, CALG_SHA_256, 0, 0, hash)) return 0;
  return CryptHashData(*hash, data, (DWORD)n, 0) != 0;
}

int signature_valid(const unsigned char *data, size_t n, const unsigned char *sig, size_t siglen) {
  HCRYPTPROV prov = 0;
  HCRYPTKEY key = 0;
  HCRYPTHASH hash = 0;
  CERT_PUBLIC_KEY_INFO *info = NULL;
  DWORD info_len = 0;
  unsigned char *rev = NULL;
  size_t i;
  int ok = 0;
  if (!release_key_present() || siglen == 0 || siglen > 2048) return 0;
  /* PROV_RSA_AES is the provider type with SHA-256 support (XP SP3 and later). */
  if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) goto done;
  if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, RELEASE_PUBLIC_KEY, RELEASE_PUBLIC_KEY_LEN,
                           CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &info_len))
    goto done;
  if (!CryptImportPublicKeyInfo(prov, X509_ASN_ENCODING, info, &key)) goto done;
  if (!sha256(prov, data, n, &hash)) goto done;
  /* CryptoAPI wants the signature little-endian; OpenSSL writes it big-endian. */
  rev = (unsigned char *)malloc(siglen);
  if (!rev) goto done;
  for (i = 0; i < siglen; i++) rev[i] = sig[siglen - 1 - i];
  ok = CryptVerifySignatureW(hash, rev, (DWORD)siglen, key, NULL, 0) != 0;
done:
  free(rev);
  if (hash) CryptDestroyHash(hash);
  if (key) CryptDestroyKey(key);
  if (info) LocalFree(info);
  if (prov) CryptReleaseContext(prov, 0);
  return ok;
}

void sha256_hex(const unsigned char *data, size_t n, char out[65]) {
  HCRYPTPROV prov = 0;
  HCRYPTHASH hash = 0;
  BYTE digest[32];
  DWORD len = sizeof digest, i;
  out[0] = 0;
  if (!CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return;
  if (sha256(prov, data, n, &hash) && CryptGetHashParam(hash, HP_HASHVAL, digest, &len, 0) && len == 32)
    for (i = 0; i < 32; i++) snprintf(out + 2 * i, 3, "%02x", digest[i]);
  if (hash) CryptDestroyHash(hash);
  CryptReleaseContext(prov, 0);
}
