# Changelog

## 0.5.0 — minimal TUI and saved API key

- Minimal inline TUI in the style of Pi / OpenCode / Claude Code, drawn with the console API (no ANSI on Windows 7): shaded user block, `$ command` proposals (yellow, red when risky), dimmed output preview (`... +N líneas · exit 0`), `pensando...`, editor between two rules, footer with folder, tokens in/out, context % and model. Plain mode when redirected or narrower than 40 columns.
- API key: optional save in the Windows Credential Manager (DPAPI, per user); `/clave` to change it, `tinyshell --logout` to delete it; rejected keys (401/403) explained without raw JSON.
- Pending console input is flushed before every approval question (a multi-line paste can no longer answer it).
- Up to 25 steps per message; prompt asks for plain text and step-by-step work.
- Agent loop moved to `src/session_win32.c`; `/ayuda` lists commands.
- Verified under wine: plain mode, Credential Manager, HTTPS 401 path, TUI layout in a pty, real `cmd.exe` commands.

## 0.4.0 — updates and installer

- `tinyshell --update` / `/actualizar`: checks the latest GitHub release, asks, downloads, verifies an RSA-3072/SHA-256 signature with the Windows CryptoAPI, self-tests the new binary on this machine (`--version` must run and report the announced, newer version), then swaps it in keeping `tinyshell.old.exe`. Never automatic; works without the API key.
- `tinyshell --rollback` restores the previous binary; `--version`, `--help`.
- `scripts/install.bat` (published with each release): installs into `%LOCALAPPDATA%\TinyShell`, adds it to the user PATH, uses a local `tinyshell.exe` next to it (USB) or downloads with curl/bitsadmin.
- `build/gen-signing-key.sh` (one-time key pair) and `build/release.sh` (test, build, sign, tag, `gh release create`; `--dry-run`).
- `http_get` for release downloads, sharing the WinHTTP/TLS 1.2 code with the API client.

## 0.3.0 — general-purpose prompt

- System prompt moved to `src/prompt_win32.c`, rewritten in English and task-agnostic: a general terminal agent for old Windows PCs (OpenCode/Pi-like) instead of a repair-only bot. Replies in the user's language.
- The prompt describes the machine detected at startup (Windows version, 32/64-bit, RAM, CPUs, current directory) instead of a hardcoded profile.
- Banner and `shell` tool description no longer assume Windows 7 or repairs.

## 0.2.0 — hardening and module split

- Split `agent.c` into `src/` modules (≤ ~200 lines each); portable core separated from Win32 adapters.
- Approval gate: over-long input no longer leaks into the `Ejecutar?` prompt; only `s`/`si`/`sí` approve; commands with control characters or over 2000 bytes are refused before asking (previously could run truncated).
- `risk_reason()` replaces `dangerous()`: shows why, normalizes `DEL/S`, quotes and paths, adds `chkdsk /f /r /x /b`, registry/service/firewall/boot changes, `^`, `>`, `&`, script hosts.
- Execution via `CreateProcessW` (stdin = NUL, 180 s timeout with job object, pipe always drained, so large outputs no longer hang).
- Command output converted from the OEM code page to UTF-8; invalid UTF-8 never reaches the JSON body (fixes HTTP 400 after Spanish `systeminfo`).
- Console uses the UTF-16 API: accented input no longer exits the program on Windows 7; API key hidden while typing.
- JSON: `\uXXXX` decoded, structure validated, empty `choices` and `null` content handled, depth/token bounds.
- History trimmed by whole exchanges (~192 KB); response body capped at 1 MB; `/reiniciar` added.
- WinHTTP requests TLS 1.2 explicitly (needs KB3140245) and prints error codes with hints.
- MinGW build links `msvcrt.dll` instead of the UCRT, which Windows 7 lacks by default.
- `collect-diagnostics.bat` writes a timestamped folder.
- Host-side regression tests (`build/test-core.sh`).
- Still not run on the target Windows 7 machine.

## 0.1.0 — prototype package

- Tiny C CLI using WinHTTP and OpenCode Go Chat Completions.
- Local CMD shell tool with per-command confirmation.
- Linux MinGW cross-build script and Windows MSVC build script.
- Added hardware profile, goals, architecture, access points, API, security, operations, roadmap and test plan documentation.
- Added optional Windows diagnostic collector batch file.
- Not yet compiled or tested against the target Windows 7 machine.
