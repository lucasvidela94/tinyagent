# Access points and interfaces

## Local entry points

| Point | Purpose | Current behavior |
|---|---|---|
| `main()` | Program entry point | Starts CLI, sets console UTF-8, asks for API key |
| Prompt `vos>` | User request | Accepts a Spanish task per line |
| Prompt `Ejecutar? [s/N]` | Local approval gate | Only `s`/`S` approves the proposed command |
| `EJECUTAR` confirmation | Extra gate | Required for commands matched by the current destructive-command heuristic |
| `/salir` or `/exit` | Session termination | Ends the process |
| `run_shell()` | Local command boundary | Calls `cmd.exe /d /s /c` and captures output |

## Network access point

- Method: `POST`
- Host: `opencode.ai`
- Path: `/zen/go/v1/chat/completions`
- Transport: HTTPS via WinHTTP, port 443
- Auth: `Authorization: Bearer <API key>`
- Content: JSON, OpenAI-compatible chat-completions messages and tool declaration
- Model: `deepseek-v4.1-flash`

The API key is requested at runtime and held in process memory. Do not commit or email it. The endpoint/model are compiled constants in `agent.c` at this stage.

## Operating-system access

- Read/execute boundary: `cmd.exe` through `_popen`.
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
