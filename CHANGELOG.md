# Changelog

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
