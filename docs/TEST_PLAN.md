# Test plan

## Build checks (Linux)

- [ ] `i686-w64-mingw32-gcc` exits successfully.
- [ ] `file tinyshell.exe` identifies a PE32 Intel 80386 Windows executable.
- [ ] No API key is embedded in the binary or source.

## Local behavior (Windows 7 x86)

- [ ] Program starts from normal CMD without missing-DLL error.
- [ ] Empty API key exits with a clear message.
- [ ] `/salir` exits cleanly.
- [ ] User rejects a proposed command; command does not execute.
- [ ] Approving `ver` returns Windows version output.
- [ ] Network failure gives a readable error and allows the user to exit.

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
