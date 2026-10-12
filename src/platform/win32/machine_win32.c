#include "win32.h"

#include <stdio.h>
#include <stdlib.h>

#include "machine.h"

static const char *windows_name(DWORD major, DWORD minor) {
  if (major == 5 && minor == 0) return "Windows 2000";
  if (major == 5) return "Windows XP";
  if (major == 6 && minor == 0) return "Windows Vista";
  if (major == 6 && minor == 1) return "Windows 7";
  if (major == 6 && minor == 2) return "Windows 8";
  if (major == 6 && minor == 3) return "Windows 8.1";
  if (major == 10) return "Windows 10/11";
  return "Windows";
}

static void add_wide(Buf *b, const wchar_t *w) {
  char tmp[MAX_PATH * 3];
  if (WideCharToMultiByte(CP_UTF8, 0, w, -1, tmp, (int)sizeof tmp, NULL, NULL)) buf_str(b, tmp);
}

/* RtlGetVersion reports the real version; GetVersionEx lies on 8.1+ without a manifest. */
void system_os(Buf *b) {
  typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW *);
  OSVERSIONINFOW v;
  RtlGetVersionFn get = NULL;
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  char num[64];
  ZeroMemory(&v, sizeof v);
  v.dwOSVersionInfoSize = sizeof v;
  if (ntdll) get = (RtlGetVersionFn)(void (*)(void))GetProcAddress(ntdll, "RtlGetVersion");
  if (!get || get(&v) != 0) {
    buf_str(b, "Windows (unknown version)");
    return;
  }
  buf_str(b, windows_name(v.dwMajorVersion, v.dwMinorVersion));
  snprintf(num, sizeof num, " (%lu.%lu build %lu", (unsigned long)v.dwMajorVersion, (unsigned long)v.dwMinorVersion,
           (unsigned long)v.dwBuildNumber);
  buf_str(b, num);
  if (v.szCSDVersion[0]) {
    buf_str(b, ", ");
    add_wide(b, v.szCSDVersion);
  }
  buf_str(b, ")");
}

void system_details(Buf *b) {
  char num[160];
  wchar_t cwd[MAX_PATH];
  SYSTEM_INFO si;
  MEMORYSTATUSEX mem;
  GetNativeSystemInfo(&si);
  mem.dwLength = sizeof mem;
  if (!GlobalMemoryStatusEx(&mem)) mem.ullTotalPhys = mem.ullAvailPhys = 0;
  system_os(b);
  snprintf(num, sizeof num, ", %s, %lu MB RAM (%lu MB free), %lu logical CPUs, current directory ",
           si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "64-bit" : "32-bit",
           (unsigned long)(mem.ullTotalPhys >> 20), (unsigned long)(mem.ullAvailPhys >> 20),
           (unsigned long)si.dwNumberOfProcessors);
  buf_str(b, num);
  if (GetCurrentDirectoryW(MAX_PATH, cwd)) add_wide(b, cwd);
  else buf_str(b, "unknown");
  buf_str(b, ".");
}

const Dialect *system_dialect(void) { return &DIALECT_CMD; }

/* The display language of Windows itself (what menus are in), not the regional format. */
const char *system_language(void) { return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH ? "es" : "en"; }
