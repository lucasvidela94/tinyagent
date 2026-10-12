#include "win32.h"

#include <wincred.h>
#include <string.h>

#include "credential.h"
#include "i18n.h"

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#endif

const char CREDENTIAL_WHERE[] = N_("the Windows Credential Manager");

static int target(const char *provider, wchar_t out[128]) {
  char t[128];
  if (strlen(provider) + 11 > sizeof t) return 0;
  strcpy(t, "TinyShell/");
  strcat(t, provider);
  return MultiByteToWideChar(CP_UTF8, 0, t, -1, out, 128) != 0;
}

int credential_load(const char *provider, char *out, size_t cap) {
  PCREDENTIALW c = NULL;
  wchar_t t[128];
  int ok = 0;
  out[0] = 0;
  if (!target(provider, t) || !CredReadW(t, CRED_TYPE_GENERIC, 0, &c)) return 0;
  if (c->CredentialBlobSize > 0 && c->CredentialBlobSize < cap) {
    memcpy(out, c->CredentialBlob, c->CredentialBlobSize);
    out[c->CredentialBlobSize] = 0;
    ok = 1;
  }
  SecureZeroMemory(c->CredentialBlob, c->CredentialBlobSize);
  CredFree(c);
  return ok;
}

int credential_save(const char *provider, const char *key) {
  CREDENTIALW c;
  wchar_t t[128];
  if (!target(provider, t)) return 0;
  ZeroMemory(&c, sizeof c);
  c.Type = CRED_TYPE_GENERIC;
  c.TargetName = t;
  c.UserName = (LPWSTR)L"api-key";
  c.CredentialBlobSize = (DWORD)strlen(key);
  c.CredentialBlob = (LPBYTE)key;
  c.Persist = CRED_PERSIST_LOCAL_MACHINE; /* this user on this PC; does not roam */
  return CredWriteW(&c, 0) != 0;
}

int credential_delete(const char *provider) {
  wchar_t t[128];
  if (!target(provider, t)) return 0;
  return CredDeleteW(t, CRED_TYPE_GENERIC, 0) || GetLastError() == ERROR_NOT_FOUND;
}
