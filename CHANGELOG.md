# Changelog

## 0.9.0 — Linux

- TinyShell for Linux: `tinyshell-linux-i386`, one static binary for x86 PCs from the Pentium up (i586, no SSE2 needed), 32- or 64-bit distros, whatever their libc or OpenSSL. It brings its own TLS (Mbed TLS 4.1 LTS) and Mozilla's root certificates, so old distros with expired roots still connect; it ran and connected on 32-bit Debian 4.0 (2007) to 8 userlands. Same agent, approval gate, login, reports and signed updates as on Windows, with `/bin/sh` risk rules and prompt guidance; the API key, if saved, goes to `~/.config/tinyshell/credentials` (mode 0600).
- `install.sh`: `curl` or `wget`, checks SHA256SUMS, installs to `~/.local/bin/tinyshell`, checks that it starts; `sh install.sh ./tinyshell-linux-i386` installs a copy brought on a USB stick (very old distros may not reach GitHub at all).
- Releases sign both binaries for `--update`.

## 0.8.0 — installer, English and Spanish, groundwork for Linux

- Windows installer, `TinyShell-Setup.exe`: per user without administrator rights, Start menu entry, optional desktop shortcut, user PATH, Programs and Features entry with an uninstaller, and "set it up now" at the end. The exe gets an icon and version details. `install.bat` now downloads and runs the installer; a loose `tinyshell.exe` remains as the portable option.
- The interface speaks English or Spanish, following the system's language, or `[ui] language = es|en` in `config.ini`. Command names are English, with the Spanish ones kept as aliases: `login`, `logout`, `report`, `telemetry` (`--show`), `/help`, `/model`, `/report`, `/reset`, `/update`, `/quit`. Approvals also accept `y`/`yes`; the strong confirmation is `RUN` in English.
- The data folder's report folder and telemetry log are now `reports\` and `telemetry.log`.
- Fixes: a failed JSON parse leaked memory (found by the new fuzzing of the JSON, SSE and HTTP parsers); the Ctrl+C note in a command's output was shown as output.
- Smaller exe from link-time optimization; checks for banned C functions, file size, include layering, docs drift and translations run before every commit and in CI.
- Groundwork for Linux: the code is now layered with a platform seam, and a static i586 Linux build exists (not published yet; see `docs/adr/0001-linux-port-platform-seam.md`).

## 0.7.0 — streaming, Ctrl+C, license

- Streaming: answers appear as they arrive (server-sent events), replacing "pensando...". Tool-call pieces are joined and the usual response is rebuilt, so approval and history work as before; providers that do not stream are handled transparently. Token counts come from the stream.
- Ctrl+C cancels instead of killing TinyShell: it aborts the model request or the running command (whole process tree), counts as "no" at an approval, ends the session cleanly at an empty prompt, and stops the current turn (remaining tool calls get a "cancelled" result).
- No fixed `temperature` in requests: some models (e.g. `kimi-k2.7-code`) only accept their default, found while testing.
- MIT license.

## 0.6.0 — login and any model

- `tinyshell login`: pick a provider (arrows + Enter), paste the API key (one `*` per character, Backspace works), pick a model from the provider's live list, done. The model list request doubles as a key check: a rejected key (401/403) is never saved. `tinyshell logout` deletes it; `/login` does the same flow inside a session.
- Any model of the provider: the list comes from `<base>/models` (38 for OpenCode Go today), so new models show up without a new release. `/modelo` opens the same list; `/modelo <nombre>` still sets one directly. `deepseek-v4.1-flash` stays only as the default.
- Menu with filtering: type to narrow the list (e.g. `glm`), 10 rows at a time, Esc cancels; numbered fallback when not in a console.
- API keys are typed with asterisks everywhere (first run too), so a right-click paste is visible on Windows 7.
- `tinyshell reporte` / `/reporte`: describe the problem; the report (system, provider, recent errors and, if accepted, the conversation) is saved locally, shown and sent only after `s`. The relay (`relay/`, Cloudflare Worker + D1) keeps the full report private and opens a public GitHub issue with the redacted summary. Without network: email the saved file to the maintainer.
- Beta telemetry: on by default while 0.x, announced on first run, `tinyshell telemetria on|off|--ver`. Metadata only (sessions, step/approval counts, tokens, duration, error codes), one short silent request per event, logged locally.
- Console input split into `console_input_win32.c`; menu in `tui_select_win32.c`; login in `login_win32.c`.

## 0.5.0 — minimal TUI, saved API key, any provider

- Provider-agnostic: `%LOCALAPPDATA%\TinyShell\config.ini` (`[provider]` name, url, model, session_header; created with examples on first run) points TinyShell at any OpenAI-compatible endpoint: OpenCode Go (default), OpenAI, OpenRouter, DeepSeek, or an LM Studio/Ollama server on the LAN over `http://` (warned at startup; no key needed). Values validated locally; one saved API key per provider; `/modelo` shows or switches the model.

- Minimal inline TUI in the style of Pi / OpenCode / Claude Code, drawn with the console API (no ANSI on Windows 7): shaded user block, `$ command` proposals (yellow, red when risky), dimmed output preview (`... +N líneas · exit 0`), `pensando...`, editor between two rules, footer with folder, tokens in/out, context % and model. Plain mode when redirected or narrower than 40 columns.
- API key: optional save in the Windows Credential Manager (DPAPI, per user); `/clave` to change it, `tinyshell --logout` to delete it; rejected keys (401/403) explained without raw JSON.
- Pending console input is flushed before every approval question (a multi-line paste can no longer answer it).
- Up to 25 steps per message; prompt asks for plain text and step-by-step work.
- Agent loop moved to `src/session_win32.c`; `/ayuda` lists commands.
- Sends the `x-opencode-session` header (random ID per conversation, new on `/reiniciar`): OpenCode Go rejects requests without it (HTTP 400 `MissingSessionID`), found in the first real test.
- Console output is fitted to the font: with a raster font (Windows 7 default), characters missing from the console code page are replaced by ASCII stand-ins (`->`, `...`, `"`, `-`, `v`; emoji dropped) instead of `?`. TrueType fonts are left alone. The prompt also asks for simple punctuation, but the code is what guarantees it.
- Verified under wine against the real API: tool call, approval, execution, tool result accepted over several round trips, final answer. Also plain mode, Credential Manager, 401 path, TUI layout in a pty.

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
