# Access points and interfaces

Command names are English; the earlier Spanish names still work as aliases (shown in
parentheses). The interface speaks Spanish or English (see "Language" below).

## Local entry points

| Point | Purpose | Current behavior |
|---|---|---|
| `main()` (`src/app/main.c`) | Program entry point | Sets the interface language, loads `config.ini`, asks for the API key without echo unless one is saved, then the chat loop |
| Editor (between two rules, footer below) | User request | One message per line in any language (max 4095 bytes; longer lines are refused). Windows: native console editing (arrows, F7 history, right-click paste). Linux: the terminal's line editing |
| Approval menu `Run it?` under the command's panel (`src/app/approval.c`; pending input is flushed first) | Local approval gate | Arrows + Enter: **Yes, once**; **Yes, always "prefix" in this session** (only for a single plain command that is not risky, see `command_prefix()`; later commands with that prefix run without asking until `/reset` or exit); **No, and tell it what to do instead** (the typed reason goes to the model). Esc rejects without a reason. Letters do nothing, so a stray key cannot answer |
| `question` tool menu | Model question | The question, its 2 to 8 options and "Other answer (type it)" in the menu (arrows, filter, Enter; Esc skips); plain mode reads a number or a name. Input is flushed first. The choice, the typed answer or "closed" goes back to the model; nothing runs |
| `[deny] commands` in `config.ini` | Pre-filter | Comma-separated rules (`diskpart, del /s, rm -r*`): a command matching one is refused before asking, and the rule is shown and sent to the model (`denied_by()`, `src/core/policy.c`). Read on every command. There are no allow rules |
| `/quit` (`/salir`, `/exit`), Ctrl+C at an empty prompt | Session termination | Ends the process |
| `tinyshell --version` / `--help` | CLI flags | Print version / usage and exit |
| `tinyshell --update` or `/update` (`/actualizar`) | Self-update | Manual only; asks; verifies the RSA signature; self-tests the new binary; keeps the previous one as `.old` |
| `tinyshell --rollback` | Undo update | Swaps the binary and its `.old` copy |
| `tinyshell login` (`--login`), `/login` (`/clave`) | Sign in | Provider menu, API key with asterisks (checked against `<base>/models`; rejected keys are not saved), model menu with filtering; saves the key and `[provider]` |
| `tinyshell logout` (`--logout`) | Forget key | Deletes the saved API key of the current provider |
| `tinyshell report` (`reporte`), `/report` (`/reporte`) | Report a problem | Asks what happened, builds the report, saves it, shows it, sends it after `y` (falls back to emailing the file) |
| `tinyshell telemetry on\|off\|--show` (`telemetria`, `--ver`) | Telemetry | Turn beta telemetry on or off; show what was sent |
| `/help` (`/ayuda`), `/model [x]` (`/modelo`) | Session commands | List the commands; pick a model from the live list or set one by name (saved to `config.ini`) |
| `/reset` (`/reiniciar`) | Session reset | Drops the conversation history, keeps the system prompt |
| `run_command()` (`shell_win32.c`, `shell_posix.c`) | Local command boundary | Windows: `CreateProcessW` of `%SystemRoot%\System32\cmd.exe /d /s /c "<cmd>"` in a job. Linux: `/bin/sh -c` in its own process group. stdin empty, 180 s limit, output capped at 24 KB |

## Language

- Spanish when Windows' display language is Spanish (`GetUserDefaultUILanguage`) or, on Linux, `LC_ALL`/`LC_MESSAGES`/`LANG` starts with `es`; English otherwise.
- `[ui] language = es | en | auto` in `config.ini` overrides it.
- What the model reads from TinyShell (command results, refusals) follows the same language; the system prompt is always English and tells the model to answer in the user's language.

## Network access point

- Configured in `config.ini` (Windows: `%LOCALAPPDATA%\TinyShell\config.ini`; Linux: `~/.config/tinyshell/config.ini`), section `[provider]`: `name`, `url`, `model`, `session_header`. Created with comments and examples on first run. `/model` shows it; `/model X` changes the model and saves it.
- Default: `POST https://opencode.ai/zen/go/v1/chat/completions`, model `deepseek-v4.1-flash`, header `x-opencode-session`.
- Transport: Windows uses WinHTTP (TLS 1.1/1.2 requested explicitly through `WINHTTP_OPTION_SECURE_PROTOCOLS`); Linux uses Mbed TLS with the system roots plus Mozilla's embedded ones. `http://` works for a model server on the LAN, with a warning at startup (key and conversation unencrypted).
- Auth: `Authorization: Bearer <API key>` when a key is set (local servers can run without one).
- Content: JSON, OpenAI-compatible chat-completions messages and the `shell` (worded for the platform's shell) and `question` tool declarations.

The API key is read from the saved store if the user chose to save it (Windows Credential Manager, target `TinyShell/<provider name>`; Linux `~/.config/tinyshell/credentials`, mode 0600), otherwise asked for at startup without echo, and held in process memory.

## Reports and telemetry access point

- `POST https://tinyshell-relay.lucasan-videla.workers.dev/v1/report`: only after the user reviews the report and answers yes. No API key is sent.
- `POST .../v1/event`: beta telemetry (`session_start`, `session_end`, `error`), metadata only, 3-4 s timeouts, silent on failure, logged locally in `telemetry.log` in the data folder. Off with `tinyshell telemetry off`.
- Local files in the data folder: `reports/report-*.txt` (exactly what a report sends), `telemetry.log`.

## Release access point (updates only)

- `GET https://github.com/lucasvidela94/tinyagent/releases/latest/download/{version.txt,<binary>,<binary>.sig}` (redirects to GitHub's CDN), only on `--update` / `/update`. `<binary>` is `tinyshell.exe` or `tinyshell-linux-i386`.
- No API key or user data is sent. Bounded: 64 bytes, 4 MB, 2 KB.

## Operating-system access

- Read/execute boundary: the platform shell, started directly (Windows: AutoRun disabled with `/d`), output through a pipe.
- Effective permissions: same user as TinyShell; if started elevated (administrator or root), commands are elevated too.
- File and system access is not restricted to a project directory: a command may reach any path that account can.
- No local listening port, HTTP server, named pipe or background service is created by design.

## Build access points

- `build/build-linux-mingw.sh` → `tinyshell.exe`; `build/build-linux-musl.sh` → `tinyshell-linux-i386`; `build/release.sh` → signed GitHub release.
- Install (Windows): `TinyShell-Setup.exe` (`build/build-installer.sh`) → `%LOCALAPPDATA%\TinyShell`, Start menu, user `PATH` (HKCU), Programs and Features entry `HKCU\...\Uninstall\TinyShell`. `scripts/install.bat` downloads and runs it.
- Install (Linux): `scripts/install.sh` → `~/.local/bin/tinyshell` (or `$TINYSHELL_DIR`), after checking `SHA256SUMS`; `sh install.sh ./file` installs a local copy.
- Windows/MSVC: `build/build-windows-msvc.bat` → `tinyshell.exe`.

## Future access points (not implemented)

- Read-only diagnostic mode and dry-run mode.
- Restricted command policy and working-directory restriction.
- Separate file read/write tools with explicit path allowlists.
- Optional structured log with secrets redacted.
