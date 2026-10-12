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
- [x] Session reset (`/reset`) and history trimming.
- [x] Exit status in every command result (`[exit=N]`). Separate stdout/stderr: not planned.
- [x] Ctrl+C cancels the request or the command without killing TinyShell.

## P2: useful harness features

- [x] Problem reports shown in full before sending (`/report`) and opt-out beta telemetry.
- [ ] Read-only file inspection with explicit path approval.
- [ ] `read` tool: bounded file reads (offset/limit, 24 KB cap like shell output), paths approved like commands.
- [ ] `grep`/`glob` tools: bounded code search over the working tree, same output cap, no recursive scans without approval.
- [ ] Carefully scoped file edits using before/after diff and backup.
- [ ] `apply` tool: edits only from a shown unified diff, one file per approval, backup kept next to the file.
- [ ] Small local project/context file describing the machine and allowed actions.
- [ ] `/plan` step list: the model states its steps up front, progress shown in the TUI, no extra processes or background agents.
- [ ] Symbol search without LSP: ctags index plus bounded `grep` (go to definition, find
  references). Native, KBs of RAM, no language-server processes on the target.
- [ ] MCP strictly as a stdio client: one server at a time, explicit allowlist, every server
  tool approved like a command, memory/time caps. Only native/small servers are usable on
  the target; Node/Python servers stay a capable-machine option behind a flag. Each tool
  still needs its regression tests and must fit the exe and memory budgets.
- [x] Endpoint/model configuration with secrets outside config (`config.ini` + Credential
      Manager, `tinyshell login`, `/model`).
- [x] CI cross-build and checks; signed releases with SHA256SUMS (`build/release.sh`).
- [x] Windows installer (`TinyShell-Setup.exe`): per user, Start menu, PATH, uninstaller.
- [x] Interface in English and Spanish, English command names with Spanish aliases.

## Next

- [ ] Saved conversations: `/history` lists past sessions, `/resume N` continues one, with
      search across them. Stored in SQLite (an amalgamation compiled in: ~600 KB, well within
      what the target machines can hold; searchable, robust to power cuts). This revisits the
      "no database" rule in AGENTS.md on purpose: it is an embedded library in the same binary,
      not a server. Encrypted per user with DPAPI on Windows, mode 0600 on Linux; a retention
      limit and a switch to turn it off, since outputs may hold sensitive data.
- [ ] Code-sign the installer and the exe (Authenticode), so Windows stops warning about an
      unknown publisher.

## P3: Linux (`docs/adr/0001-linux-port-platform-seam.md`)

- [x] Layered `src/` with the platform seam and an enforced include rule.
- [x] Step 0: `sys` seam, portable `update`, `cancel` without Windows handles.
- [x] Shell dialect as data: risk rules, prompt guidance and tool description per shell.
- [ ] `http_last_error` without WinHTTP wording.
- [x] POSIX adapters: sys, console, shell and cancel (tested natively; TUI checked in a pty).
- [x] POSIX adapters: HTTP with Mbed TLS, settings, credentials, machine, verify.
- [x] Static i586 musl build (no SSE2), checked: real HTTPS update check and a real model
      session with a tool call on this host.
- [ ] Test on old distros: runs and connects over HTTPS on 32-bit Debian 4.0 (2007) to 8 userlands
      in Docker; old kernels still need QEMU or real hardware.
- [ ] `install.sh`, dual signed release, site section, CI job for the Linux build.
- [x] API key storage on Linux (0600 file) and the hardware floor (i586) decided.

## Explicitly out of scope for the low-spec target

- Local model inference, Electron, browser UI, Docker, persistent agents and heavyweight runtimes.
- Bundled MCP/LSP servers: external servers cost more RAM than the whole binary and break
  the single-file promise. Allowed only as strict clients (MCP over stdio, ctags instead of
  LSP) under the P2 conditions above, never as bundled servers.
- Background agents and subagent trees: the CLI stays synchronous and memory-bounded; `/plan`
  covers multi-step work without extra processes.
