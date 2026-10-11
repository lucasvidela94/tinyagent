#include "win32.h"

#include <wincred.h>
#include <string.h>

#include "credential.h"

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#endif

#define TARGET L"TinyShell/OpenCodeGo"

int credential_load(char *out, size_t cap) {
  PCREDENTIALW c = NULL;
  int ok = 0;
  out[0] = 0;
  if (!CredReadW(TARGET, CRED_TYPE_GENERIC, 0, &c)) return 0;
  if (c->CredentialBlobSize > 0 && c->CredentialBlobSize < cap) {
    memcpy(out, c->CredentialBlob, c->CredentialBlobSize);
    out[c->CredentialBlobSize] = 0;
    ok = 1;
  }
  SecureZeroMemory(c->CredentialBlob, c->CredentialBlobSize);
  CredFree(c);
  return ok;
}

int credential_save(const char *key) {
  CREDENTIALW c;
  ZeroMemory(&c, sizeof c);
  c.Type = CRED_TYPE_GENERIC;
  c.TargetName = (LPWSTR)TARGET;
  c.UserName = (LPWSTR)L"opencode-go";
  c.CredentialBlobSize = (DWORD)strlen(key);
  c.CredentialBlob = (LPBYTE)key;
  c.Persist = CRED_PERSIST_LOCAL_MACHINE; /* this user on this PC; does not roam */
  return CredWriteW(&c, 0) != 0;
}

int credential_delete(void) {
  return CredDeleteW(TARGET, CRED_TYPE_GENERIC, 0) || GetLastError() == ERROR_NOT_FOUND;
}
