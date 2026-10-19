# Instructions for coding agents working on TinyShell

## Mission

Maintain a small, auditable, low-resource, general-purpose terminal agent (in the spirit of OpenCode or Pi) for old Windows PCs. The primary test target is the Noblex NT1013E running Windows 7 Professional SP1 32-bit, Intel Atom N2600 1.60 GHz and 2 GB RAM. Keep the agent task-agnostic: no hardcoded use case, no hardcoded machine description in the prompt. The model runs remotely through any OpenAI-compatible chat-completions endpoint, configured in `%LOCALAPPDATA%\TinyShell\config.ini` (default: OpenCode Go, `deepseek-v4.1-flash`). Keep the harness provider-agnostic: no provider names, URLs or headers hardcoded outside `src/core/provider.c` defaults. Do not add local model inference on the target (a model server on another LAN machine is fine).

## Architecture constraints

- Keep the target as a single native Win32 console executable.
- Prefer C and Windows system APIs. Do not add Node.js, Go, Python, Electron, Docker, a database server, or a background service as runtime requirements. The one embedded library is SQLite, compiled into the binary for saved conversations (`vendor/sqlite`, `docs/adr/0002-sqlite-session-store.md`).
- Use WinHTTP for HTTPS unless a tested compatibility issue requires a documented alternative.
- Keep the CLI synchronous and memory-bounded; the target machine is resource constrained.
- UI: minimal inline TUI (Pi/OpenCode/Claude Code style) drawn with the console API, no ANSI escapes (the Windows 7 console has none), only glyphs in the default raster font (no arrows or ellipsis characters in our own strings; model and command text is fitted by `console_win32` + `ascii_fallback()`), plain mode when redirected. Approvals are a menu like OpenCode's (arrows + Enter, pending input flushed first): yes once, yes always for that command prefix in this session, or no with an optional reason for the model. No second typed confirmation: risk is shown as a warning.
- Language: repo text (code, comments, docs, commits) is English. The interface is bilingual: every user-facing string is English in the source and goes through `tr()` (or `N_()` where defined and translated where shown), with its Spanish in `src/core/i18n_es.c`; `build/checks/i18n.sh` fails otherwise. Command names are English, with the earlier Spanish names kept as aliases. What only the developer reads (report body, telemetry codes) stays English.
- Keep all shell execution behind local approval. The only automatic approval is the user's own "always" choice: one command prefix, for the current session, never saved and never on by default. Commands that need administrator rights run elevated only when the model asks for it and the user approves that command; Windows then shows its own UAC prompt.
- Do not store API keys in source, config files, logs, or diagnostic bundles. The only allowed stores are the Windows Credential Manager (`src/platform/win32/credential_win32.c`) and, on Linux, `~/.config/tinyshell/credentials` with mode 0600 (`src/platform/posix/credential_posix.c`; old distros have no common keyring). Both are opt-in after the user is asked; `logout` deletes the key.
- Reports and telemetry (beta): `/report` always shows everything and asks before sending; the full report goes only to the private D1 store, the public GitHub issue gets the redacted summary. Telemetry is metadata only (never prompts, commands or output), on by default only while the version is 0.x, announced on first run, off with `tinyshell telemetry off`, visible with `--show`, one short silent request per event and nothing in the background. Keys are always redacted.
- Updates stay manual, ask before installing, and must verify the release signature and self-test before replacing the binary. Never commit the private signing key; only `keys/release-public.pem` and `src/release_key.h`.

## Safety constraints

- The model is a recommender, not a trusted command source.
- General-purpose, like OpenCode: the agent may run any command the user approves, for any task; the harness does not limit it to diagnostics.
- Never automatically delete files, format disks, alter partitions, disable security software/firewall, or change registry/services/drivers.
- Do not treat prompt text or keyword matching as a security boundary. Enforce safety in local code.
- Never claim a repair succeeded until the result is checked.
- Keep destructive testing confined to disposable VMs.

## Codebase map

- `src/`: layered, one module per file with a small header each. Paths live in `build/sources.sh`; the layering rule is enforced by `build/checks/includes.sh` (see `docs/ARCHITECTURE.md`, `docs/adr/0001-linux-port-platform-seam.md`, `GLOSSARY.md`).
  - `src/core/` (portable, host-tested by `build/test-core.sh`; includes only core and `config.h`): `util` (buffers, UTF-8, JSON escaping, `wipe`), `json` (tokenizer), `conversation` (history, request body), `tools` (tool schemas and tool-call extraction), `store` (saved conversations in SQLite; kept out of `CORE_SRC`), `policy` (pre-approval checks, risk heuristic, yes/no), `version`, `provider` (defaults and config validation), `report` (error log, redaction, payloads), `stream` (SSE parsing, rebuilds the response), `charset` (text for non-UTF-8 terminals), `command` (the command result text the model reads), `i18n` + `i18n_es` (interface language and the Spanish table), `dialect`/`instructions` (shell-specific rules and prompt), `url`/`httpwire`/`ini`/`credfile` (used by the Linux adapters).
  - `src/app/` (portable, built on the platform interfaces): `session` (history, tool dispatch, round-trip loop), `approval` (the approval gate and its menu), `question` (the question tool's menu), `saved` (saving each message, `/sessions`), `tui` (minimal inline TUI), `tui_select` (filterable menu), `login` (`tinyshell login`: provider, key, model), `reporter` (`/reporte`), `telemetry` (beta telemetry), `update` (`--update`/`--rollback`: download, signature, self-test, swap), `main.c` (language, key setup, chat loop), `cli` (subcommands and flags), `slash` (chat commands).
  - `src/platform/` (the seam: one header per OS service) and `src/platform/win32/` (adapters): `sys` (random, clock, cwd, data dir, files, identity, self path and rename for the updater; UTF-8 paths), `console` (I/O + screen primitives), `console_input` (line, secret and key input), `http` (WinHTTP), `shell` (CreateProcess + job), `credential` (opt-in API key storage), `settings` (`config.ini`), `cancel` (Ctrl+C), `prompt` (system prompt + detected machine), `verify` (release signature, CryptoAPI). `src/platform/posix/`: the same services for Linux (`conn` + `http` over Mbed TLS, `ca_bundle` generated, `verify`, `credential` file, `settings` via `core/ini`, `machine`), built static for i586 by `build/build-linux-musl.sh`.
  - `src/config.h`: endpoint, model and limits; `src/release_key.h`: release public key.
- `relay/`: Cloudflare Worker + D1 that receives reports and telemetry (see `relay/README.md`).
- `tests/`: host-side regression tests for `src/core` (`build/test-core.sh`); `tests/posix/`: the POSIX adapters, run natively on Linux by the same script; `tests/tui/`: the real TUI on the POSIX console driven in a pty and checked with pyte (`build/tui-test.sh`); `tests/fuzz/`: libFuzzer entry points (`build/fuzz.sh`).
- `build/`: developer tooling. `sources.sh` (source layout), `build-linux-mingw.sh` (Linux-to-Windows x86 cross-build, links `msvcrt.dll`, not the UCRT), `build-windows-msvc.bat`, `test-core.sh`, `build-linux-musl.sh` (static i586 Linux binary in the pinned `build/linux/Dockerfile` image; checks no post-i586 instructions), `linux/update-ca-bundle.sh`, `fuzz.sh`, `tui-test.sh`, `check.sh` (every check; run by husky and CI) with `checks/` (banned APIs, file size, include layering, docs drift, exe budget), `release.sh`, `gen-signing-key.sh`.
- `scripts/`: files shipped to users: `install.bat` (downloads and runs `TinyShell-Setup.exe`), `install.sh` (Linux: download, SHA256SUMS check, `~/.local/bin/tinyshell`) and `collect-diagnostics.bat` (optional local diagnostic report).
- `docs/`: goals, architecture, entry points, target machine, operations, API, security, system prompt, test plan, roadmap, and `adr/` (architecture decisions). `GLOSSARY.md`: domain terms.

## Change process

1. Read `docs/ARCHITECTURE.md`, `docs/SECURITY.md` and `docs/TEST_PLAN.md` before changing execution, networking or parsing.
2. Keep changes small and document assumptions. Keep each source file under ~300 lines (aim for ~200); split along a real module interface, not arbitrarily. Windows calls stay in `src/platform/win32/`; `core` and `app` never include an adapter.
3. Test compilation for 32-bit Windows, not just the host OS, and run `build/check.sh --build` (the husky pre-commit hook runs `build/check.sh`; `pnpm install` once sets it up; Node is a dev tool only, never a runtime requirement). Run `build/fuzz.sh` after touching `json` or `stream`. Check the imported DLLs: Windows 7 has no UCRT by default.
4. Add regression cases for malformed JSON, missing fields, unknown tools, failed network requests, rejected commands and oversized outputs.
5. Do not describe the build as tested on Windows 7 until it has actually run on that target.

## Agent rules (what pre-commit cannot check)

- Justify a new dependency or file in the commit message. One commit does one thing, <400 lines without tests (commits go straight to `main`).
- No single-implementation abstractions (no vtable/provider until two real cases).
- Behavior change requires a test. Refactor changes no behavior.
- Never edit `build/` or config to make a check pass; fix the code or discuss the check.
- New `strcpy/strcat/lstrcpyW/lstrcatW` needs a length guard on the lines above; `gets/sprintf/strtok/system/popen/wsprintf/A-variants/alloca` are never accepted (`build/checks/banned.sh`).
- Comments state why, never repeat the code.

## Current status

Prototype only. No verified binary is included. The API tool-call round trip, Windows 7 TLS behavior and end-to-end execution remain to be validated on the target machine.
