# Instructions for coding agents working on TinyShell

## Mission

Maintain a small, auditable, low-resource, general-purpose terminal agent (in the spirit of OpenCode or Pi) for old Windows PCs. The primary test target is the Noblex NT1013E running Windows 7 Professional SP1 32-bit, Intel Atom N2600 1.60 GHz and 2 GB RAM. Keep the agent task-agnostic: no hardcoded use case, no hardcoded machine description in the prompt. The model runs remotely through any OpenAI-compatible chat-completions endpoint, configured in `%LOCALAPPDATA%\TinyShell\config.ini` (default: OpenCode Go, `deepseek-v4.1-flash`). Keep the harness provider-agnostic: no provider names, URLs or headers hardcoded outside `src/provider.c` defaults. Do not add local model inference on the target (a model server on another LAN machine is fine).

## Architecture constraints

- Keep the target as a single native Win32 console executable.
- Prefer C and Windows system APIs. Do not add Node.js, Go, Python, Electron, Docker, a database, or a background service as runtime requirements.
- Use WinHTTP for HTTPS unless a tested compatibility issue requires a documented alternative.
- Keep the CLI synchronous and memory-bounded; the target machine is resource constrained.
- UI: minimal inline TUI (Pi/OpenCode/Claude Code style) drawn with the console API, no ANSI escapes (the Windows 7 console has none), only glyphs in the default raster font (no arrows or ellipsis characters in our own strings; model and command text is fitted by `console_win32` + `ascii_fallback()`), plain mode when redirected. Approvals are typed (`s` + Enter), never a single key.
- Keep all shell execution behind local approval. Never implement auto-approval.
- Do not store API keys in source, config files, logs, or diagnostic bundles. The only allowed store is the Windows Credential Manager (`src/credential_win32.c`), opt-in after the user is asked; `--logout` deletes it.
- Reports and telemetry (beta): `/reporte` always shows everything and asks before sending; the full report goes only to the private D1 store, the public GitHub issue gets the redacted summary. Telemetry is metadata only (never prompts, commands or output), on by default only while the version is 0.x, announced on first run, off with `tinyshell telemetria off`, visible with `--ver`, one short silent request per event and nothing in the background. Keys are always redacted.
- Updates stay manual, ask before installing, and must verify the release signature and self-test before replacing the binary. Never commit the private signing key; only `keys/release-public.pem` and `src/release_key.h`.

## Safety constraints

- The model is a recommender, not a trusted command source.
- Prefer read-only diagnostics first.
- Never automatically delete files, format disks, alter partitions, disable security software/firewall, or change registry/services/drivers.
- Do not treat prompt text or keyword matching as a security boundary. Enforce safety in local code.
- Never claim a repair succeeded until the result is checked.
- Keep destructive testing confined to disposable VMs.

## Codebase map

- `src/`: one module per file, small header interface each. Portable C (testable on any host): `util` (buffers, UTF-8, JSON escaping), `json` (tokenizer), `conversation` (history, request body, tool-call extraction), `policy` (pre-approval checks, risk heuristic, yes/no). Win32 adapters: `console_win32` (UTF-16 console I/O), `http_win32` (WinHTTP), `shell_win32` (CreateProcess + pipe). `prompt_win32` (task-agnostic system prompt + machine detected at startup). `verify_win32` (release signature, CryptoAPI) and `update_win32` (`--update`/`--rollback`). `version` and `provider` (portable: provider defaults and config validation). `settings_win32` (`config.ini`). `console_win32` (I/O + screen primitives), `tui_win32` (minimal inline TUI: user block, proposal, output preview, editor between rules, footer), `credential_win32` (opt-in API key storage), `console_input_win32` (line, secret and key input), `tui_select_win32` (filterable menu), `login_win32` (`tinyshell login`: provider, key, model), `reporter_win32` (`/reporte`), `telemetry_win32` (beta telemetry), `report` (portable: error log, redaction, payloads), `session_win32` (history, approval gate, round-trip loop). `main.c`: CLI flags, key setup, chat loop and slash commands. `config.h`: endpoint, model and limits.
- `relay/`: Cloudflare Worker + D1 that receives reports and telemetry (see `relay/README.md`).
- `tests/`: host-side regression tests for the portable modules (`build/test-core.sh`).
- `build/build-linux-mingw.sh`: Linux-to-Windows x86 cross-build (links `msvcrt.dll`, not the UCRT).
- `build/build-windows-msvc.bat`: Windows/MSVC build.
- `scripts/collect-diagnostics.bat`: optional local diagnostic report.
- `docs/`: project goals, architecture, entry points, target machine, operations, API, security, test plan and roadmap.

## Change process

1. Read `docs/ARCHITECTURE.md`, `docs/SECURITY.md` and `docs/TEST_PLAN.md` before changing execution, networking or parsing.
2. Keep changes small and document assumptions. Keep each source file under ~300 lines (aim for ~200); split along a real module interface, not arbitrarily. Windows calls stay in `*_win32.c` / `main.c` so the rest stays host-testable.
3. Test compilation for 32-bit Windows, not just the host OS, and run `build/test-core.sh`. Check the imported DLLs: Windows 7 has no UCRT by default.
4. Add regression cases for malformed JSON, missing fields, unknown tools, failed network requests, rejected commands and oversized outputs.
5. Do not describe the build as tested on Windows 7 until it has actually run on that target.

## Current status

Prototype only. No verified binary is included. The API tool-call round trip, Windows 7 TLS behavior and end-to-end execution remain to be validated on the target machine.
