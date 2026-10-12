#include "win32.h"

#include <wincrypt.h>

#include <stdlib.h>
#include <string.h>

#include "sys.h"

const char SYS_EOL[] = "\r\n";
const char SYS_HOME_LABEL[] = "%USERPROFILE%";
const char SYS_RELEASE_ASSET[] = "tinyshell.exe";

static unsigned long last_error;

static int kept(int ok) { /* records the error before free() can touch it */
  if (!ok) last_error = GetLastError();
  return ok;
}

static wchar_t *wide(const char *s) { /* UTF-8 -> UTF-16, NULL if invalid. Caller frees. */
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  wchar_t *w;
  if (n <= 0) return NULL;
  w = (wchar_t *)xrealloc(NULL, (size_t)n * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
  return w;
}

static void add_utf8(Buf *b, const wchar_t *w) {
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  char *s;
  if (n <= 1) return;
  s = (char *)xrealloc(NULL, (size_t)n);
  WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL);
  buf_str(b, s);
  free(s);
}

void sys_random(unsigned char *out, size_t n) {
  HCRYPTPROV prov = 0;
  size_t i;
  int ok = CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
           CryptGenRandom(prov, (DWORD)n, out);
  if (prov) CryptReleaseContext(prov, 0);
  if (!ok)
    for (i = 0; i < n; i++) out[i] = (unsigned char)((GetTickCount() >> (i % 4 * 8)) ^ (i * 37));
}

unsigned long sys_ms(void) { return GetTickCount(); }

void sys_cwd(Buf *out) {
  wchar_t w[MAX_PATH];
  DWORD n = GetCurrentDirectoryW(MAX_PATH, w);
  if (n && n < MAX_PATH) add_utf8(out, w);
}

void sys_identity(Buf *host, Buf *user, Buf *home) {
  wchar_t w[MAX_PATH];
  DWORD n = MAX_PATH;
  if (GetComputerNameW(w, &n)) add_utf8(host, w);
  n = MAX_PATH;
  if (GetUserNameW(w, &n)) add_utf8(user, w);
  n = GetEnvironmentVariableW(L"USERPROFILE", w, MAX_PATH);
  if (n && n < MAX_PATH) add_utf8(home, w);
}

/* %LOCALAPPDATA%\TinyShell (XP: %USERPROFILE%\Local Settings\Application Data\TinyShell). */
static int data_dir(wchar_t *dir) {
  DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
  if (!n || n >= MAX_PATH) {
    n = GetEnvironmentVariableW(L"USERPROFILE", dir, MAX_PATH);
    if (!n || n + 40 >= MAX_PATH) return 0;
    lstrcatW(dir, L"\\Local Settings\\Application Data");
  }
  if (lstrlenW(dir) + 12 >= MAX_PATH) return 0;
  lstrcatW(dir, L"\\TinyShell");
  CreateDirectoryW(dir, NULL); /* fine if it already exists */
  return 1;
}

int sys_data_path(const char *name, Buf *out) {
  wchar_t dir[MAX_PATH], *wn = wide(name), *p;
  int ok = 0;
  if (!wn || !data_dir(dir) || (size_t)lstrlenW(dir) + (size_t)lstrlenW(wn) + 2 > MAX_PATH) goto done;
  for (p = wn; *p; p++)
    if (*p == L'/') { /* a folder inside the data dir: create it */
      *p = 0;
      lstrcatW(dir, L"\\");
      lstrcatW(dir, wn);
      CreateDirectoryW(dir, NULL);
      dir[lstrlenW(dir) - lstrlenW(wn) - 1] = 0;
      *p = L'\\';
    }
  lstrcatW(dir, L"\\");
  lstrcatW(dir, wn);
  add_utf8(out, dir);
  ok = 1;
done:
  free(wn);
  return ok;
}

char *sys_read_file(const char *path, size_t max, size_t *len) {
  wchar_t *w = wide(path);
  HANDLE f = w ? CreateFileW(w, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL)
               : INVALID_HANDLE_VALUE;
  DWORD size, got = 0;
  char *data;
  free(w);
  if (f == INVALID_HANDLE_VALUE) return NULL;
  size = GetFileSize(f, NULL);
  if (size == INVALID_FILE_SIZE) size = 0;
  if (size > max) {
    SetFilePointer(f, -(LONG)max, NULL, FILE_END); /* keep the end */
    size = (DWORD)max;
  }
  data = (char *)xrealloc(NULL, (size_t)size + 1);
  if (!ReadFile(f, data, size, &got, NULL)) got = 0;
  CloseHandle(f);
  data[got] = 0;
  if (len) *len = got;
  return data;
}

int sys_write_file(const char *path, const char *data, size_t n, int flags) {
  int append = flags & SYS_APPEND; /* SYS_EXECUTABLE: any file can run on Windows */
  wchar_t *w = wide(path);
  DWORD done = 0;
  HANDLE f = w ? CreateFileW(w, append ? FILE_APPEND_DATA : GENERIC_WRITE, FILE_SHARE_READ, NULL,
                             append ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)
               : INVALID_HANDLE_VALUE;
  int ok;
  free(w);
  if (f == INVALID_HANDLE_VALUE) return 0;
  ok = WriteFile(f, data, (DWORD)n, &done, NULL) && done == n;
  return CloseHandle(f) && ok;
}

int sys_self_path(Buf *out) {
  wchar_t w[MAX_PATH];
  DWORD n = GetModuleFileNameW(NULL, w, MAX_PATH);
  if (!kept(n && n < MAX_PATH)) return 0;
  add_utf8(out, w);
  return 1;
}

/* A running .exe cannot be overwritten or deleted, but it can be renamed. */
int sys_rename(const char *from, const char *to) {
  wchar_t *f = wide(from), *t = wide(to);
  int ok = kept(f && t && MoveFileExW(f, t, MOVEFILE_REPLACE_EXISTING));
  free(f);
  free(t);
  return ok;
}

int sys_remove(const char *path) {
  wchar_t *w = wide(path);
  int ok = kept(w && DeleteFileW(w));
  free(w);
  return ok;
}

int sys_exists(const char *path) {
  wchar_t *w = wide(path);
  int ok = kept(w && GetFileAttributesW(w) != INVALID_FILE_ATTRIBUTES);
  free(w);
  return ok;
}

unsigned long sys_error(void) { return last_error; }
