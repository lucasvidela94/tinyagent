#include "win32.h"

#include <stdio.h>
#include <stdlib.h>

#include "cancel.h"
#include "cancel_win32.h"
#include "command.h"
#include "i18n.h"
#include "config.h"
#include "shell.h"
#include "util.h"

/* Child output is in the OEM code page unless it is already UTF-8 (e.g. after chcp 65001). */
static char *to_utf8(char *s) {
  int wn, n;
  wchar_t *w;
  char *o;
  if (utf8_valid(s)) return s;
  wn = MultiByteToWideChar(CP_OEMCP, 0, s, -1, NULL, 0);
  if (wn <= 0) return s; /* jesc() still replaces invalid bytes before sending */
  w = (wchar_t *)xrealloc(NULL, (size_t)wn * sizeof(wchar_t));
  MultiByteToWideChar(CP_OEMCP, 0, s, -1, w, wn);
  n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  o = (char *)xrealloc(NULL, (size_t)(n > 0 ? n : 1));
  if (n <= 0 || !WideCharToMultiByte(CP_UTF8, 0, w, -1, o, n, NULL, NULL)) o[0] = 0;
  free(w);
  free(s);
  return o;
}

static wchar_t *command_line(const char *cmd) {
  int wn = MultiByteToWideChar(CP_UTF8, 0, cmd, -1, NULL, 0);
  wchar_t *line;
  if (wn <= 0) return NULL;
  line = (wchar_t *)xrealloc(NULL, ((size_t)wn + 24) * sizeof(wchar_t));
  lstrcpyW(line, L"cmd.exe /d /s /c \"");
  MultiByteToWideChar(CP_UTF8, 0, cmd, -1, line + lstrlenW(line), wn);
  lstrcatW(line, L"\"");
  return line;
}

/* Drains the pipe until every writer is gone, cmd.exe has exited (plus a grace period for
 * background children holding the pipe), or the timeout hits. Returns 1 on timeout. */
static int collect(HANDLE rd, HANDLE proc, Buf *out, size_t *dropped) {
  DWORD start = GetTickCount(), exited_at = 0;
  for (;;) {
    DWORD avail = 0, got = 0;
    char buf[4096];
    if (!PeekNamedPipe(rd, NULL, 0, NULL, &avail, NULL)) return 0;
    if (avail) {
      if (!ReadFile(rd, buf, avail < sizeof buf ? avail : sizeof buf, &got, NULL) || !got) return 0;
      buf_add_capped(out, buf, got, MAX_OUTPUT, dropped); /* keep reading so the child never blocks */
      continue;
    }
    if (GetTickCount() - start > CMD_TIMEOUT_MS) return 1;
    if (WaitForSingleObject(proc, 50) == WAIT_OBJECT_0) {
      if (!exited_at) exited_at = GetTickCount();
      else if (GetTickCount() - exited_at > 500) return 0;
      Sleep(50);
    }
  }
}

static char *finish(Buf *out, size_t dropped, int timed_out, DWORD code) {
  char *text, *res;
  size_t k;
  for (k = 0; k < out->len; k++)
    if (!out->p[k]) out->p[k] = ' '; /* NUL bytes would cut the text short */
  text = to_utf8(buf_take(out));
  res = command_result(text, dropped, timed_out, cancel_requested(), code);
  free(text);
  return res;
}

char *run_command(const char *cmd) {
  wchar_t app[MAX_PATH + 16], *line;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  HANDLE rd, wr, nul, job;
  DWORD code = 0;
  UINT sl = GetSystemDirectoryW(app, MAX_PATH);
  int timed_out, in_job;
  size_t dropped = 0;
  Buf out = {0};
  char msg[192];

  if (!sl || sl >= MAX_PATH) return xstrdup(tr("cmd.exe was not found. Nothing was run."));
  lstrcatW(app, L"\\cmd.exe");
  if (!(line = command_line(cmd))) return xstrdup(tr("The command has an invalid encoding. Nothing was run."));
  if (!CreatePipe(&rd, &wr, &sa, 0)) {
    free(line);
    return xstrdup(tr("Could not create the output pipe. Nothing was run."));
  }
  SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
  nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
  ZeroMemory(&si, sizeof si);
  si.cb = sizeof si;
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = nul;
  si.hStdOutput = wr;
  si.hStdError = wr;
  job = CreateJobObjectW(NULL, NULL);
  if (!CreateProcessW(app, line, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
    snprintf(msg, sizeof msg, tr("Could not start cmd.exe (error %lu). Nothing was run."),
             (unsigned long)GetLastError());
    pi.hProcess = pi.hThread = NULL;
  }
  free(line);
  CloseHandle(wr); /* keep only the child's copy so the read side sees EOF */
  if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
  if (!pi.hProcess) {
    CloseHandle(rd);
    if (job) CloseHandle(job);
    return xstrdup(msg);
  }
  /* The job lets a timeout kill the whole tree. Assignment fails on Windows 7 when this
   * process already runs inside a job; then only cmd.exe itself can be terminated. */
  in_job = job && AssignProcessToJobObject(job, pi.hProcess);
  ResumeThread(pi.hThread);
  cancel_watch_process(in_job ? job : NULL, pi.hProcess); /* Ctrl+C kills the tree */
  CloseHandle(pi.hThread);

  timed_out = collect(rd, pi.hProcess, &out, &dropped);
  cancel_release_process();
  if (timed_out) {
    if (in_job) TerminateJobObject(job, 1);
    else TerminateProcess(pi.hProcess, 1);
  }
  WaitForSingleObject(pi.hProcess, 5000);
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  CloseHandle(rd);
  if (job) CloseHandle(job);
  return finish(&out, dropped, timed_out, code);
}
