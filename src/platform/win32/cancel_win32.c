#include "win32.h"

#include <winhttp.h>

#include "cancel.h"
#include "cancel_win32.h"

static CRITICAL_SECTION lock;
static volatile LONG requested, input_interrupted;
static HINTERNET http;
static HANDLE job, process;

/* A pending ReadConsole does not return on Ctrl+C everywhere, so when nothing else is
 * running, an Enter key is injected and the reader learns it was an interruption. */
static void interrupt_input(void) {
  INPUT_RECORD ev[2];
  DWORD written;
  int i;
  ZeroMemory(ev, sizeof ev);
  for (i = 0; i < 2; i++) {
    ev[i].EventType = KEY_EVENT;
    ev[i].Event.KeyEvent.bKeyDown = i == 0;
    ev[i].Event.KeyEvent.wRepeatCount = 1;
    ev[i].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    ev[i].Event.KeyEvent.uChar.UnicodeChar = L'\r';
  }
  InterlockedExchange(&input_interrupted, 1);
  WriteConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), ev, 2, &written);
}

static BOOL WINAPI on_ctrl(DWORD type) {
  if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE; /* close/logoff: default */
  InterlockedExchange(&requested, 1);
  EnterCriticalSection(&lock);
  if (!http && !job && !process) interrupt_input();
  if (http) {
    WinHttpCloseHandle(http); /* makes the pending WinHTTP call fail at once */
    http = NULL;
  }
  if (job) TerminateJobObject(job, 1);
  else if (process) TerminateProcess(process, 1);
  LeaveCriticalSection(&lock);
  return TRUE; /* handled: do not end the process */
}

void cancel_init(void) {
  InitializeCriticalSection(&lock);
  SetConsoleCtrlHandler(on_ctrl, TRUE);
}

int cancel_requested(void) { return requested != 0; }
int cancel_take_input_interrupt(void) { return InterlockedExchange(&input_interrupted, 0) != 0; }
void cancel_reset(void) { InterlockedExchange(&requested, 0); }

void cancel_watch_http(HINTERNET request) {
  EnterCriticalSection(&lock);
  http = request;
  LeaveCriticalSection(&lock);
  if (requested) on_ctrl(CTRL_C_EVENT); /* Ctrl+C arrived just before registering */
}

int cancel_release_http(HINTERNET request) {
  int owned;
  EnterCriticalSection(&lock);
  owned = http == request;
  if (owned) http = NULL;
  LeaveCriticalSection(&lock);
  return owned;
}

void cancel_watch_process(HANDLE j, HANDLE p) {
  EnterCriticalSection(&lock);
  job = j;
  process = p;
  LeaveCriticalSection(&lock);
}

void cancel_release_process(void) {
  EnterCriticalSection(&lock);
  job = process = NULL;
  LeaveCriticalSection(&lock);
}
