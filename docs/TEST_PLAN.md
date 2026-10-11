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

## Under wine (Linux, before the target)

- [x] `--version`, `--help`, unknown flag, `--update` without release key.
- [x] Plain mode (piped input): key prompt, save to Credential Manager, reuse on next start, `--logout`, `/ayuda`, unknown command, real HTTPS request to opencode.ai with a fake key → 401 handled with `/clave` hint.
- [x] TUI in a pseudo-terminal (rendered with pyte): editor between rules and footer, user block with background and wrapping, editor erased after submit, correct placement when the buffer is full.
- [x] Real commands through `cmd.exe`: output preview (`... +851 líneas · exit=0`), `pause` returns at once (stdin = NUL), risky command asks `EJECUTAR`.

## TUI (Windows 7 x86)

- [ ] Rules (`─`) and `·` render in the default raster font; colors readable on the default black background.
- [ ] Editor: arrows/F7 history and right-click paste work; text appears inside the editor.
- [ ] Pasting several lines does not answer `ejecutar?` (input flushed).
- [ ] After ~300 lines (buffer full) the editor still lands in the right place.

## Install and update (Windows 7 x86)

- [ ] `install.bat` from a USB folder installs the local copy, prints SHA-256, adds the user PATH; a new CMD finds `tinyshell`.
- [ ] The bitsadmin one-liner works (or fails with the USB hint) on the target.
- [ ] `tinyshell --version` prints `tinyshell X.Y.Z`.
- [ ] `--update` with no newer release says it is up to date.
- [ ] `--update` to a newer signed release: signature verified, self-test passes, binary swapped, `.old.exe` kept.
- [ ] A release whose `.sig` does not match is refused with nothing changed.
- [ ] `--rollback` restores the previous binary; a second `--rollback` rolls forward.
- [x] Signing pipeline (`build/release.sh --dry-run` with a throwaway key): OpenSSL verifies the signature; a one-byte change fails it.

## API behavior

- [x] Simple text response appears. (wine, real API, 2026-10-11)
- [x] Tool call for a read-only command appears with command and reason. (wine, real API)
- [x] Tool result is accepted by API and the model produces a final response; several round trips with the raw assistant message kept in history are accepted. (wine, real API)
- [x] Missing `x-opencode-session` is rejected with HTTP 400; with it, requests succeed. (wine, real API)
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
