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

## Future hardening priorities

- Add a strict default-deny allowlist for diagnostic commands.
- Parse and display structured command risk; block destructive classes instead of only asking twice.
- ~~Add output limits, API response size limits, and history trimming.~~ Done: 24 KB output, 1 MB response, ~192 KB history (whole exchanges dropped).
- Use `CreateProcess` with explicit argument handling rather than composing a shell command string where feasible (`CreateProcess` is used, but still through `cmd.exe /c`).
- Add an explicit `--read-only` mode.
- Redact secrets and network identifiers from optional logs.
- Test JSON parser and API response handling with malformed and oversized inputs.
