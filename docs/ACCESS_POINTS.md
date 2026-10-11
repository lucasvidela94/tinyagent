# Access points and interfaces

## Local entry points

| Point | Purpose | Current behavior |
|---|---|---|
| `main()` (`src/main.c`) | Program entry point | Starts CLI, asks for API key without echo; console I/O uses the UTF-16 console API (no code-page change) |
| Editor (between two rules, footer below) | User request | One message per line in any language (max 4095 bytes; longer lines are refused). Native console editing: arrows, F7 history, right-click paste |
| Prompt `ejecutar? [s/N]` (pending input is flushed first) | Local approval gate | Only `s`, `si` or `sí` (any case) approves. Anything else, EOF or an over-long line rejects |
| `EJECUTAR` confirmation | Extra gate | Required when `risk_reason()` (`src/policy.c`) flags the command; the reason is shown |
| `/salir` or `/exit` | Session termination | Ends the process |
| `tinyshell --version` / `--help` | CLI flags | Print version / usage and exit |
| `tinyshell --update` or `/actualizar` | Self-update | Manual only; asks; verifies RSA signature; self-tests the new binary; keeps `tinyshell.old.exe` |
| `tinyshell --rollback` | Undo update | Swaps `tinyshell.exe` and `tinyshell.old.exe` |
| `tinyshell login` | Sign in | Provider menu, API key with asterisks (checked against `<base>/models`; rejected keys are not saved), model menu with filtering; saves key + `[provider]` |
| `tinyshell logout` (`--logout`) | Forget key | Deletes the saved API key of the current provider |
| `tinyshell reporte` / `/reporte` | Report a problem | Asks what happened, builds the report, saves it, shows it, sends after `s` (falls back to emailing the file) |
| `tinyshell telemetria on\|off\|--ver` | Telemetry | Turn beta telemetry on/off; show what was sent |
| `/ayuda`, `/login`, `/modelo [x]` | Session commands | Show commands; run the login flow; pick a model from the live list or set one by name (saved to `config.ini`) |
| `/reiniciar` | Session reset | Drops the conversation history, keeps the system prompt |
| `run_command()` (`src/shell_win32.c`) | Local command boundary | `CreateProcessW` of `%SystemRoot%\System32\cmd.exe /d /s /c "<cmd>"`, stdin = NUL, 180 s limit, output capped at 24 KB |

## Network access point

- Configured in `%LOCALAPPDATA%\TinyShell\config.ini`, section `[provider]`: `name`, `url`, `model`, `session_header`. Created with comments and examples on first run; edit with Notepad. `/modelo` shows it; `/modelo X` changes the model and saves it.
- Default: `POST https://opencode.ai/zen/go/v1/chat/completions`, model `deepseek-v4.1-flash`, header `x-opencode-session`.
- Transport: WinHTTP, `https://` (TLS 1.2 requested) or `http://` (warned at startup: key and conversation unencrypted; meant for a model server on the LAN).
- Auth: `Authorization: Bearer <API key>` when a key is set (local servers can run without one).
- Content: JSON, OpenAI-compatible chat-completions messages and tool declaration.

The API key is read from the Windows Credential Manager if the user chose to save it (target `TinyShell/<provider name>`, one per provider), otherwise requested at startup without echo, and held in process memory. Do not commit or email it. The endpoint/model are compiled constants in `src/config.h` at this stage. TLS 1.1/1.2 are requested explicitly via `WINHTTP_OPTION_SECURE_PROTOCOLS`.

## Reports and telemetry access point

- `POST https://tinyshell-relay.lucasan-videla.workers.dev/v1/report`: only after the user reviews the report and answers `s`. No API key is sent.
- `POST .../v1/event`: beta telemetry (`session_start`, `session_end`, `error`), metadata only, 3-4 s timeouts, silent on failure, logged locally in `%LOCALAPPDATA%\TinyShell\telemetria.log`. Off with `tinyshell telemetria off`.
- Local files: `%LOCALAPPDATA%\TinyShell\reportes\reporte-*.txt` (exactly what a report sends), `telemetria.log`.

## Release access point (updates only)

- `GET https://github.com/lucasvidela94/tinyagent/releases/latest/download/{version.txt,tinyshell.exe,tinyshell.exe.sig}` (redirects to GitHub's CDN), only on `--update` / `/actualizar`.
- No API key or user data is sent. Bounded: 64 bytes, 4 MB, 2 KB.

## Operating-system access

- Read/execute boundary: `cmd.exe` started with `CreateProcessW` (AutoRun disabled with `/d`), output through an anonymous pipe.
- Effective permissions: same user and integrity level as TinyShell; if started elevated, commands are elevated too.
- File/system access is not restricted to a project directory. A command may access any path available to that Windows account.
- No local listening port, HTTP server, named pipe, or background service is created by design.

## Build access points

- Linux: `build/build-linux-mingw.sh` → `tinyshell.exe`; `build/release.sh` → signed GitHub release.
- Install: `scripts/install.bat` → `%LOCALAPPDATA%\TinyShell`, user `PATH` (HKCU) and `TINYSHELL_HOME`.
- Windows/MSVC: `build/build-windows-msvc.bat` → `tinyshell.exe`.

## Future access points (not implemented)

- Config file for model/endpoint (without storing secrets).
- Read-only diagnostic mode (`--diagnose`) and dry-run mode.
- Restricted command policy and working-directory restriction.
- Separate file-read/write tools with explicit path allowlists.
- Optional structured log with secrets redacted.
