# Access points and interfaces

## Local entry points

| Point | Purpose | Current behavior |
|---|---|---|
| `main()` (`src/main.c`) | Program entry point | Starts CLI, asks for API key without echo; console I/O uses the UTF-16 console API (no code-page change) |
| Prompt `vos>` | User request | Accepts a Spanish task per line (max 4095 bytes; longer lines are refused, not truncated) |
| Prompt `Ejecutar? [s/N]` | Local approval gate | Only `s`, `si` or `sí` (any case) approves. Anything else, EOF or an over-long line rejects |
| `EJECUTAR` confirmation | Extra gate | Required when `risk_reason()` (`src/policy.c`) flags the command; the reason is shown |
| `/salir` or `/exit` | Session termination | Ends the process |
| `/reiniciar` | Session reset | Drops the conversation history, keeps the system prompt |
| `run_command()` (`src/shell_win32.c`) | Local command boundary | `CreateProcessW` of `%SystemRoot%\System32\cmd.exe /d /s /c "<cmd>"`, stdin = NUL, 180 s limit, output capped at 24 KB |

## Network access point

- Method: `POST`
- Host: `opencode.ai`
- Path: `/zen/go/v1/chat/completions`
- Transport: HTTPS via WinHTTP, port 443
- Auth: `Authorization: Bearer <API key>`
- Content: JSON, OpenAI-compatible chat-completions messages and tool declaration
- Model: `deepseek-v4.1-flash`

The API key is requested at runtime and held in process memory. Do not commit or email it. The endpoint/model are compiled constants in `src/config.h` at this stage. TLS 1.1/1.2 are requested explicitly via `WINHTTP_OPTION_SECURE_PROTOCOLS`.

## Operating-system access

- Read/execute boundary: `cmd.exe` started with `CreateProcessW` (AutoRun disabled with `/d`), output through an anonymous pipe.
- Effective permissions: same user and integrity level as TinyShell; if started elevated, commands are elevated too.
- File/system access is not restricted to a project directory. A command may access any path available to that Windows account.
- No local listening port, HTTP server, named pipe, or background service is created by design.

## Build access points

- Linux: `build/build-linux-mingw.sh` → `tinyshell.exe`.
- Windows/MSVC: `build/build-windows-msvc.bat` → `tinyshell.exe`.

## Future access points (not implemented)

- Config file for model/endpoint (without storing secrets).
- Read-only diagnostic mode (`--diagnose`) and dry-run mode.
- Restricted command policy and working-directory restriction.
- Separate file-read/write tools with explicit path allowlists.
- Optional structured log with secrets redacted.
