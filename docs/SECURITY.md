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
7. Keep the API key private. The current program asks at runtime and does not intentionally persist it; still, do not run a binary from an untrusted source.

## Important implementation caveats

- `risk_reason()` (`src/policy.c`) is only a keyword heuristic; it can miss destructive commands and can flag harmless commands. It is not a security boundary. It flags `^`, `>`, `&`, delete/format/partition tools, `chkdsk /f /r /x /b`, registry/service/firewall/boot changes, process killing, permission changes, installers and script hosts.
- Checks enforced in local code before the user is even asked (`command_problem()`): only the `shell` tool, non-empty command, at most 2000 bytes, no control characters or line breaks. What is displayed for approval is exactly what runs.
- Over-long typed lines are consumed and refused, so leftover input can never answer the approval prompt.
- Since all commands already ask for confirmation, that confirmation is the primary safeguard. Read the full command before answering yes.
- Commands still run through `cmd.exe /d /s /c` (now via `CreateProcessW`, stdin = NUL, 180 s limit). Quoting and shell metacharacters can change what runs.
- The process has broad access to the files available to its Windows account; no filesystem sandbox exists.
- A compromised or malicious API response must never be granted automatic execution.

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
