# Codebase architecture

## Architectural style

**Single-process, synchronous CLI / thin remote-agent harness.** Native C with the system's
HTTP stack (WinHTTP on Windows; Mbed TLS, statically linked, on Linux) and the system shell
(`cmd.exe` or `/bin/sh`). No local server, daemon, database, framework or managed runtime.
The remote provider proposes; the executable is the local mediator that asks for approval and
runs the command.

```text
Person
  │ request in any language / local approval
  ▼
CLI (src/app/main.c)
  ├── message history (JSON)
  ├── HTTP(S) client ───────────────────► OpenAI-compatible API (config.ini)
  │                                        default: OpenCode Go / deepseek-v4.1-flash
  │◄──────────────── streamed answer / tool call
  ├── small JSON tokenizer + SSE parser
  ├── local approval gate
  └── shell runner (cmd.exe or /bin/sh, stdin empty, timeout)
           │ output + exit code
           └──────────────────────────────► history / next turn
```

## Source map

`src/` is layered (paths in `build/sources.sh`; the decision is
`docs/adr/0001-linux-port-platform-seam.md`):

```
src/config.h, release_key.h   build-time constants
src/core/                     portable, host-tested (build/test-core.sh)
src/app/                      the program: session, TUI, login, reports, update, main
src/platform/                 the platform seam: one header per OS service
src/platform/win32/           Windows adapters
src/platform/posix/           Linux adapters (static i586 binary: build/build-linux-musl.sh)
```

Dependency rule, checked on every commit by `build/checks/includes.sh`: `core` includes only
`core` and `config.h`; `platform/*.h` include `core`; `app` includes `core`, `app` and
`platform`, never an adapter; an adapter includes `core`, `platform` and its own folder, never
`app` or another platform. OS headers are allowed only in their own adapters. `#include`s
carry no paths.

One module per file, each with a small header. No file grows past ~300 lines (aim for ~200);
a growing file is split along a real interface, not by size.

### Core (portable, host-tested)

| Module | Interface | Responsibility |
|---|---|---|
| `src/config.h` | constants | endpoint, model, limits (`MAX_HTTP`, `MAX_HISTORY`, `MAX_OUTPUT`, `MAX_COMMAND`, `CMD_TIMEOUT_MS`…) |
| `src/core/util.c` | `Buf`, `utf8_valid`, `jesc`, `wipe` | buffers, UTF-8 checks, JSON escaping (invalid bytes become `�`), wiping secrets |
| `src/core/json.c` | `json_parse/key/index/str` | bounded tokenizer (depth, tokens), `\uXXXX` to UTF-8; fuzzed |
| `src/core/stream.c` | `stream_feed`, `stream_response` | SSE streaming: live text, joins tool-call pieces, rebuilds the plain response; fuzzed |
| `src/core/conversation.c` | `History`, `build_request` | history bounded by eliding old outputs, then by whole exchanges; request body |
| `src/core/tools.c` | `tools_json`, `ToolCall` | the tools offered to the model (shell described per dialect, question) and tool-call extraction |
| `src/core/policy.c` | `tool_problem`, `risk_reason`, `denied_by`, `is_yes` | refusals before the user is asked (commands and questions), risk heuristic per dialect, the user's deny rules, approval answers |
| `src/core/dialect.h`, `dialect_cmd.c`, `dialect_sh.c` | `DIALECT_CMD`, `DIALECT_SH` | what depends on the shell, as data: risk rules and prompt guidance for `cmd.exe` and `/bin/sh` |
| `src/core/instructions.c` | `instructions` | the task-agnostic prompt instructions, with the dialect's parts |
| `src/core/command.c` | `command_result` | the text a command result goes back to the model as, the same on every platform |
| `src/core/provider.c` | `provider_defaults`, `provider_set` | provider (name, URL, model, session header), defaults and `config.ini` validation |
| `src/core/report.c` | `errlog_*`, `redact`, `report_json`, `event_json` | recent errors, redaction of keys and personal data, relay payloads |
| `src/core/version.c` | `version_compare`, `version_from_output`, `version_sibling` | X.Y.Z versions, `--version` output, the updater's `.new`/`.old` file names |
| `src/core/i18n.c`, `i18n_es.c` | `tr`, `i18n_set`, `i18n_parse` | the interface language: English source strings, the Spanish table, checked by `build/checks/i18n.sh` |
| `src/core/charset.c` | `charset_encode`, `charset_decode`, `charset_from_locale` | text for terminals that are not UTF-8 (`LANG=C` or Latin-1 on old distros) |
| `src/core/url.c`, `httpwire.c` | `url_parse`, `httpwire_*`, `http_status_hint` | URLs and HTTP/1.1 (request, incremental response, chunked) for Linux; fuzzed |
| `src/core/ini.c`, `credfile.c` | `ini_get/set`, `credfile_get/set` | `config.ini` editing and the API key file, for Linux |

### App (portable, built on the platform interfaces)

| Module | Interface | Responsibility |
|---|---|---|
| `src/app/main.c` | `main` | interface language, API key, chat loop |
| `src/app/cli.c` | `cli_run` | subcommands (`login`, `logout`, `report`, `telemetry`) and flags (`--version/--update/--rollback`), with the earlier Spanish names as aliases |
| `src/app/slash.c` | `slash_run` | chat commands (`/help`, `/model`, `/login`, `/report`, `/reset`, `/update`, `/quit`), with Spanish aliases |
| `src/app/session.c` | `session_send` | system prompt, history, tool dispatch (`run_tool`), approval gate (`decide`), turn loop, token counts |
| `src/app/question.c` | `question_ask` | the `question` tool: the question, its options in the menu and a typed "other answer" |
| `src/app/tui.c`, `tui_select.c` | `tui_*`, `tui_select` | minimal inline TUI: user block, proposal, output summary, editor between rules, footer; arrow-key menu with filtering; plain mode without a console |
| `src/app/login.c` | `login_run`, `login_pick_model` | `tinyshell login`: provider, key (checked against `/models`), model from the live list |
| `src/app/reporter.c` | `reporter_run` | `/reporte`: builds, saves, shows and sends the report |
| `src/app/telemetry.c` | `telemetry_init/event/error` | beta telemetry: notice, `telemetry on/off/--show`, local log of what was sent |
| `src/app/update.c` | `update_run`, `update_rollback` | `--update`: version.txt, download, signature, self-test, swap with the `.old` sibling; `--rollback` |

### Platform adapters

Each header in `src/platform/` has a Windows and a Linux adapter.

| Service | Windows (`src/platform/win32/`) | Linux (`src/platform/posix/`) |
|---|---|---|
| `sys` | `sys_win32.c`: CryptoAPI random, `GetTickCount`, `%LOCALAPPDATA%\TinyShell`, UTF-16 paths, renaming a running exe | `sys_posix.c`: `/dev/urandom`, `CLOCK_MONOTONIC`, `~/.config/tinyshell` (0700), files 0600, `/proc/self/exe` |
| `console`, `console_input` | `console_win32.c`, `console_input_win32.c`: UTF-16 console API without changing the code page, raster-font glyph fitting | `console_posix.c`, `console_input_posix.c`: ANSI escapes, cursor row via `ESC[6n`, termios, locale charset |
| `shell` | `shell_win32.c`: `cmd.exe /d /s /c` via `CreateProcessW`, job object, OEM output to UTF-8 | `shell_posix.c`: `/bin/sh -c` in its own process group; timeout and Ctrl+C kill the group |
| `cancel` | `cancel_win32.c`: console handler closes the request or kills the job; guarded handles | `cancel_posix.c`: SIGINT without `SA_RESTART`, SIGPIPE ignored |
| `http` | `http_win32.c`: WinHTTP, TLS 1.2 requested explicitly, readable errors | `conn_posix.c` + `http_posix.c`: TCP with timeouts, Mbed TLS with system plus embedded Mozilla roots (`ca_bundle.c`), redirects never to http and never carrying the key to another host |
| `credential` | `credential_win32.c`: Credential Manager (DPAPI) | `credential_posix.c`: `~/.config/tinyshell/credentials`, 0600, written atomically |
| `settings` | `settings_win32.c`: `GetPrivateProfileStringW` | `settings_posix.c`: `core/ini` |
| `machine` | `machine_win32.c`: `RtlGetVersion`, RAM, CPUs; the `cmd` dialect | `machine_posix.c`: os-release/lsb-release/issue, kernel, `/proc/meminfo`; the `sh` dialect |
| `verify` | `verify_win32.c`: RSA-3072/SHA-256 with the CryptoAPI | `verify_posix.c`: Mbed TLS |

Other files:

- `build/build-linux-mingw.sh`: Linux-to-Windows x86 cross-build with LTO; links `msvcrt.dll`, not the UCRT.
- `build/build-windows-msvc.bat`: build from the MSVC Developer Command Prompt.
- `build/build-linux-musl.sh`: static i586 Linux binary in the pinned `build/linux/Dockerfile` image; rejects post-i586 instructions.
- `build/test-core.sh`, `build/tui-test.sh`, `build/fuzz.sh`, `build/check.sh`: host tests (POSIX adapters included on Linux), the TUI in a pty, fuzzing, every check.
- `build/gen-signing-key.sh`, `build/release.sh`: release key pair (private key outside the repo) and signed releases.
- `relay/`: Cloudflare Worker + D1 that receives reports (public issue with a summary, private full report) and telemetry.
- `build/installer/tinyshell.nsi`, `build/build-installer.sh`: the Windows installer (`TinyShell-Setup.exe`, NSIS in Docker): per user, Start menu, PATH, Programs and Features, uninstaller.
- `scripts/install.bat`: downloads and runs the installer (kept for one-line commands published earlier).
- `scripts/collect-diagnostics.bat`: separate local diagnostics; needs no API.

## Data flow

1. The person writes a goal.
2. The program adds it to the history and sends it over HTTPS.
3. The model streams a normal answer or tool calls: `shell` or `question`.
4. For `shell`, the program shows the command and the reason. For `question`, it shows the question and its options in the menu (plus "other answer"), and the choice goes back as the result; nothing runs.
5. A rejection goes back to the model as such; an approved command runs in the platform shell.
6. The result goes back as a tool message and the model may continue, up to 25 steps per message (`MAX_TURNS`).
7. Before each request the history is trimmed to ~192 KB: first old command outputs over 1 KB are replaced by a short note that keeps the exit status (all but the last two, oldest first), then whole exchanges are dropped (a `tool_calls` message is never separated from its results). Eliding before dropping follows the context-management results in arXiv:2609.20804, where cheap elision matched summarization in accuracy at lower cost.

## Constraints / design choices

- Targets: 32-bit x86. Windows from XP-era APIs up (tested target: Windows 7 SP1); Linux from i586 CPUs, any libc.
- HTTP: WinHTTP on Windows, so TLS and certificates depend on the Windows 7 installation; Mbed TLS 4.1 LTS on Linux, with its own roots, so an old distro's expired bundle does not matter (TLS 1.2 is requested explicitly on Windows).
- JSON: small embedded tokenizer, no external dependency; treated as sensitive code, tested and fuzzed against unexpected responses.
- Execution: the platform shell in its own job or process group; not a sandbox, and command quoting has limits.
- Memory: response ≤ 1 MB (`MAX_HTTP`), history ~192 KB (`MAX_HISTORY`, soft: the last two command outputs and the last exchange are always kept), command output ≤ 24 KB (`MAX_OUTPUT`), command ≤ 2000 bytes (`MAX_COMMAND`), JSON ≤ 20 000 tokens (`MAX_TOKENS`) and 64 levels (`MAX_DEPTH`), commands stopped after 180 s (`CMD_TIMEOUT_MS`).
- Text: UTF-8 internally; the Windows console uses the UTF-16 API and command output is converted from the OEM code page; on Linux, output to non-UTF-8 terminals goes through `core/charset`.

## Future changes

- No microservices or extra runtimes: the target machines must stay simple.
- Tools are a closed set in `src/core/tools.c` (schema and argument parsing) and `tool_problem()` (limits), host-tested; `session.c` dispatches by name with a plain `if`. A new tool adds its schema, fields and checks there and one branch in the session, not a registry or function pointers.
