# Roadmap

## P0: required before real use

- [x] Cross-compile as PE32 x86 and inspect imports (`build/build-linux-mingw.sh`, CI).
- [ ] Test start-up and WinHTTP TLS on the actual Windows 7 SP1 machine.
- [ ] Validate a chat-completions request and one tool-call round trip on the target. (Done under
      wine with the real OpenCode Go API, streaming included; not yet on Windows 7.)
- [ ] Verify on the target that a rejection always means zero local command execution.
- [x] Bounded history and response limits (code + host tests; still to observe on target).

## P1: safety and robustness

- [ ] Explicit read-only mode and default-deny command allowlist.
- [ ] Safer process creation where feasible. (Partial: `CreateProcessW` + job + timeout, still
      `cmd.exe /c`.)
- [x] HTTP status/error reporting without leaking credentials.
- [x] Host tests for JSON, token boundaries, missing fields and malformed responses; libFuzzer
      on the JSON and SSE parsers (`build/fuzz.sh`).
- [x] Session reset (`/reiniciar`) and history trimming.
- [x] Exit status in every command result (`[exit=N]`). Separate stdout/stderr: not planned.
- [x] Ctrl+C cancels the request or the command without killing TinyShell.

## P2: useful harness features

- [x] Problem reports shown in full before sending (`/reporte`) and opt-out beta telemetry.
- [ ] Read-only file inspection with explicit path approval.
- [ ] Carefully scoped file edits using before/after diff and backup.
- [ ] Small local project/context file describing the machine and allowed actions.
- [x] Endpoint/model configuration with secrets outside config (`config.ini` + Credential
      Manager, `tinyshell login`, `/modelo`).
- [x] CI cross-build and checks; signed releases with SHA256SUMS (`build/release.sh`).

## P3: Linux (`docs/adr/0001-linux-port-platform-seam.md`)

- [x] Layered `src/` with the platform seam and an enforced include rule.
- [x] Step 0: `sys` seam, portable `update`, `cancel` without Windows handles.
- [ ] `http_last_error` without WinHTTP wording; shell dialect as data (with the POSIX shell).
- [ ] POSIX adapters: sys, console, shell and cancel; HTTP with Mbed TLS; credentials; update.
- [ ] Static i386 musl build, `install.sh`, dual signed release, site section.
- [ ] Decide API key storage on Linux and the hardware floor (open questions in the ADR).

## Explicitly out of scope for the low-spec target

- Local model inference, Electron, browser UI, Docker, persistent agents and heavyweight runtimes.
