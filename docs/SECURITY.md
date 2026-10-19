# Security model and limitations

## Trust boundaries

- The local user is the decision-maker.
- The remote model is an untrusted recommender: output may be wrong, outdated, or manipulated by text found on the PC.
- The local command runner has the current user's OS permissions.
- Network requests send user prompts, command output and machine information to the API provider.

## Rules

1. Review every proposed command before approving it.
2. Do not run as administrator for routine diagnosis.
3. Never paste Windows passwords, API keys from other services, personal documents, or sensitive logs into prompts.
4. Do not disable antivirus, firewall, certificate checks or Windows security features to make the tool work.
5. Back up important files before any repair that writes to disk, changes registry/services/drivers, or alters partitions.
6. Treat any command that uses `&`, `&&`, `|`, redirection, `for`, `call`, PowerShell, scripts, or encoded content as a compound command and inspect every part.
7. Keep the API key private. It is saved only if you say yes, in the Windows Credential Manager (encrypted with your Windows account via DPAPI; other accounts cannot read it, but any program running as you can). `tinyshell logout` removes it; `tinyshell login` saves it (after checking it against the provider). Do not run a binary from an untrusted source.

## Important implementation caveats

- `risk_reason()` (`src/core/policy.c`) is only a keyword heuristic; it can miss destructive commands and can flag harmless commands. It is not a security boundary. It flags `^`, `>`, `&`, delete/format/partition tools, `chkdsk /f /r /x /b`, registry/service/firewall/boot changes, process killing, permission changes, installers and script hosts.
- Checks enforced in local code before the user is even asked (`tool_problem()`): only the `shell` and `question` tools; a non-empty command of at most 2000 bytes with no control characters or line breaks; a question of at most 1000 bytes with 2 to 8 one-line options. What is displayed for approval is exactly what runs. A question runs nothing: the answer only goes back to the model.
- The user's deny rules (`[deny] commands` in `config.ini`, `denied_by()`) refuse matching commands before asking. They use the same word matching as `risk_reason()`, so they share its limits (a command can be spelled to slip past them); they narrow what reaches the approval gate, they do not replace it.
- Over-long typed lines are consumed and refused, and pending console input is flushed before every approval question, so leftover or multi-line pasted input can never answer it.
- Since all commands already ask for confirmation, that confirmation is the primary safeguard. Read the full command before answering yes.
- Commands still run through `cmd.exe /d /s /c` (now via `CreateProcessW`, stdin = NUL, 180 s limit). Quoting and shell metacharacters can change what runs.
- The process has broad access to the files available to its Windows account; no filesystem sandbox exists.
- A compromised or malicious API response must never be granted automatic execution.

## Provider configuration

- `config.ini` never holds secrets; API keys stay in the Credential Manager, one per provider `name`.
- Values are validated in local code (`src/core/provider.c`): URL must be `http://` or `https://` without spaces; `session_header` only letters, digits and `-` (no header injection); `name` only safe characters (it becomes the credential target). Invalid values are reported and the default is used.
- `http://` URLs are allowed for LAN model servers but warned at every start: the key and the whole conversation (including command output) travel unencrypted.
- Changing the URL sends your prompts and command output to that server: only point it at services you trust.

## Reports and telemetry

- **Reports** are sent only after the user sees them and answers yes. The public summary (version, Windows, provider/model, description, recent error codes) has the PC name, Windows user and home folder replaced; the full report (adds this session's conversation, if the user accepts) is stored only in the private D1 database and is readable only by the maintainer. Anything key-shaped (`sk-...`, `Bearer ...`) is removed from both, and the relay redacts again before writing to GitHub. Everything sent is also saved locally in `reportes\`.
- **Telemetry** during the 0.x beta is on by default, announced on the first run, and off with `tinyshell telemetry off`. It carries metadata only: install ID (random), version, Windows version, provider/model, step and approval counts, token counts, duration, and error kind/code/stage. Never prompts, commands, outputs, paths or keys. Each event is one request with 3-4 s timeouts, silent if it fails; nothing runs in the background. `tinyshell telemetria --ver` shows exactly what was sent.
- The relay stores IPs only as a salted hash and rate-limits reports and events.

## Updates and installation

- Updates are never automatic and always ask. They do not need the API key.
- Authenticity: each release binary carries an RSA-3072 / SHA-256 signature made with a private key that lives only on the maintainer's machine (`build/gen-signing-key.sh`). The binary embeds the public key (`src/release_key.h`) and verifies with the Windows CryptoAPI before writing anything. A compromised GitHub account alone cannot ship an update.
- Downgrade protection: the version that must be newer is the one the *signed* binary reports when self-tested (`--version`), not only `version.txt`.
- Self-test: the new binary must start on this Windows before it replaces the old one; the old one is kept for `--rollback`.
- First install is trust-on-first-use (HTTPS from GitHub, like `curl | sh`). Compare the SHA-256 the installer prints with the release page.
- If the private key leaks, installed copies would accept updates signed with it: rotate by shipping a release with a new public key while the old key is still trusted, and treat the old key as burned.
- `install.bat` only touches `%LOCALAPPDATA%\TinyShell`, the user `PATH` under HKCU and `TINYSHELL_HOME`; never the system PATH and never as administrator.

## Future hardening priorities

- Add a strict default-deny allowlist for diagnostic commands.
- Parse and display structured command risk; block destructive classes instead of only asking twice.
- ~~Add output limits, API response size limits, and history trimming.~~ Done: 24 KB output, 1 MB response, ~192 KB history (whole exchanges dropped).
- Use `CreateProcess` with explicit argument handling rather than composing a shell command string where feasible (`CreateProcess` is used, but still through `cmd.exe /c`).
- Add an explicit `--read-only` mode.
- Redact secrets and network identifiers from optional logs.
- Test JSON parser and API response handling with malformed and oversized inputs.
