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

## From OpenCode

Ideas taken from [OpenCode](https://github.com/anomalyco/opencode) (October 2026), whose Go
plan is TinyShell's default provider, sized for old machines: no extra processes, no
auto-approval, every tool bounded and approved like a command.

Visibility:

- [ ] Get TinyShell listed among OpenCode Go's "Validated Clients" (docs page `go.mdx`): it
      already meets the three requirements (its own User-Agent `TinyShell/x.y.z`, a stable
      `x-opencode-session` per conversation, typical agent traffic). A PR to their docs.
- [ ] Add it to OpenCode's ecosystem page, awesome-opencode and opencode.cafe, framed as the
      option for old and low-resource PCs. The README and site say it is not affiliated with
      OpenCode (their naming guidance; the name already avoids "opencode").
- [ ] `README.es.md`: the README fully in Spanish, linked from the top, as OpenCode does with
      its translated READMEs. Keeps the main README English without leaving users out.

Features:

- [ ] `/compact`: the model summarizes the older part of the conversation into one message.
      Long sessions keep their context while using fewer tokens and less RAM than trimming.
- [ ] `question` tool: the model asks a multiple-choice question shown with the existing menu
      (arrows, filter) instead of guessing; the answer goes back as the tool result.
- [ ] Machine notes, like OpenCode's `AGENTS.md` and `/init`: `/init` lets the model propose
      a short `TINYSHELL.md` (what this PC is, what was already fixed, what not to touch),
      saved in the data folder after the user approves it and read at every session start.
      Refines the "small local context file" item above.
- [ ] Custom commands: prompt templates in the data folder (`commands/<name>.md`) run as
      `/<name> [args]`, e.g. a technician's checklist for a slow PC. Text only, no code.
- [ ] `tinyshell run "<request>"`: one request from the command line (approvals still asked),
      for desktop shortcuts and scripts.
- [ ] `/export`: write the conversation to a text file the user can keep or send.
- [ ] `/stats`: tokens per session and per day, from the local counters.
- [ ] Deny rules in `config.ini` (command patterns refused before asking). Never allow rules:
      every command still needs approval.
- [ ] `webfetch` tool: fetch one URL as text (bounded size, http(s) only, approved like a
      command), e.g. to read a driver's download page or an error code's explanation.
- [ ] `tinyshell uninstall` on Linux (Windows has Programs and Features).

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
- [x] `install.sh`, dual signed release (0.9.0) and the site's Linux section.
- [ ] CI job for the Linux build (the toolchain image takes ~8 minutes; cache it in GHCR).
- [x] API key storage on Linux (0600 file) and the hardware floor (i586) decided.

## Explicitly out of scope for the low-spec target

- Local model inference, Electron, browser UI, Docker, persistent agents and heavyweight runtimes.
- Bundled MCP/LSP servers: external servers cost more RAM than the whole binary and break
  the single-file promise. Allowed only as strict clients (MCP over stdio, ctags instead of
  LSP) under the P2 conditions above, never as bundled servers.
- Background agents and subagent trees: the CLI stays synchronous and memory-bounded; `/plan`
  covers multi-step work without extra processes.
