# Test plan

## Build checks (Linux)

- [x] `build/build-linux-mingw.sh` exits successfully (`-Werror`).
- [x] `file tinyshell.exe` identifies a PE32 Intel 80386 Windows executable.
- [x] Imports only `KERNEL32.dll`, `msvcrt.dll`, `WINHTTP.dll` (no UCRT).
- [x] No API key is embedded in the binary or source.
- [x] `build/test-core.sh` passes under ASan/UBSan: malformed/oversized JSON, missing fields, null content, `\u` escapes, invalid UTF-8, unknown tools, malformed arguments, control characters, oversized commands, risk heuristic, approval answers, history trimming, output cap.

## Local behavior (Windows 7 x86)

- [ ] Program starts from normal CMD without missing-DLL error.
- [ ] Empty API key exits with a clear message.
- [ ] `/salir` exits cleanly.
- [ ] User rejects a proposed command; command does not execute.
- [ ] Approving `ver` returns Windows version output.
- [ ] Network failure gives a readable error (WinHTTP code + hint) and allows the user to exit.
- [ ] Typing accented text (`¿qué pasa con la red?`) does not exit the program and reaches the model intact.
- [ ] Spanish `systeminfo` output shows accents correctly and the next API request succeeds (no HTTP 400).
- [ ] Pasting a line longer than 4095 bytes is refused and does not answer the next `Ejecutar?` prompt.
- [ ] Answering `seguro que no` or `salir` to `Ejecutar?` rejects.
- [ ] A large output (`dir /s C:\Windows`) is truncated with a note and does not hang.
- [ ] A command waiting for input (`pause`) returns immediately; `ping -n 300 127.0.0.1` is killed at 180 s.
- [ ] `/reiniciar` clears the history; a long session keeps working (history trimmed).

## API behavior

- [ ] Simple text response appears.
- [ ] Tool call for a read-only command appears with command and reason.
- [ ] Tool result is accepted by API and the model produces a final response.
- [ ] HTTP 401 and 429 are reported usefully.
- [ ] Malformed JSON does not trigger shell execution.
- [ ] Unknown tool name is never executed.

## Safety checks

- [ ] No command executes without local approval.
- [ ] Destructive test cases are exercised only in a disposable VM, never on the real PC.
- [ ] API key is not printed or written to disk.
- [ ] No background listener/service is created.

## Acceptance

Do not call it production-ready until all P0 tests pass on the target Windows 7 x86 installation.
