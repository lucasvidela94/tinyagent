# Glossary

TinyShell's domain terms. Code and docs use these names.

- **Session** (`Session`): one conversation with the model: history, provider, API key,
  counters and session ID.
- **Provider** (`Provider`): an OpenAI-compatible chat-completions endpoint (name, URL, model
  and session header). OpenCode Go is the default preset.
- **Turn**: one round trip with the model. A user message may take up to `MAX_TURNS` turns.
- **Proposal**: a command the model asks to run through the `shell` tool, with its reason.
  The model only proposes.
- **Approval**: the typed local confirmation (`s` + Enter, plus `EJECUTAR` when risky)
  without which nothing runs.
- **Risk**: the local reason (`risk_reason`) a command needs the strong confirmation. Code
  decides it, not the prompt.
- **Shell dialect** (`Dialect`): what depends on the platform's shell: risk rules, prompt
  guidance and the tool description (`cmd.exe` on Windows, `/bin/sh` on Linux).
- **Core**: the portable modules in `src/core/`, tested on the host.
- **Platform seam**: the headers in `src/platform/` that each platform implements (`sys`,
  `console`, `console_input`, `http`, `shell`, `credential`, `settings`, `cancel`, `machine`,
  `verify`).
- **Platform adapter**: the implementation of those headers for one platform
  (`src/platform/win32/`, `src/platform/posix/`).
- **Release**: a binary signed with the RSA release key, plus `version.txt`, the signature and
  `SHA256SUMS`. `--update` installs only what verifies and passes the self-test.
- **Report**: what `/reporte` sends: the redacted public summary (a GitHub issue) and the
  private full report (D1).
- **Event**: beta telemetry: metadata only, never what the user types.
- **Relay**: the Cloudflare Worker that receives reports and events.
