# Security policy

## Reporting a vulnerability

**Do not open a public issue** for a vulnerability (command execution without approval, bypassing the update signature, stealing API keys, injection through the model's answers).

Email **lucasan.videla@gmail.com** with:

- The version (`tinyshell --version`) and the affected OS.
- A description of the problem and its likely impact.
- Minimal steps to reproduce it (or a proof of concept).
- If you can, the component you think is at fault (`src/...`).

You will get an acknowledgement and word when a fix or release is out. If you want, you are credited in `CHANGELOG.md`.

## Scope

- The binaries (`tinyshell.exe`, `tinyshell-linux-i386`), `scripts/install.bat` and the `--update` flow (RSA signature and self-test).
- The local approval gate (`src/core/policy.c`, `src/core/command.c`, `src/app/session.c`).
- API key handling (`src/platform/win32/credential_win32.c`, `src/platform/posix/credential_posix.c`) and `config.ini`.
- The HTTPS client on Linux (`src/platform/posix/conn_posix.c`, `src/platform/posix/http_posix.c`).
- The report and telemetry relay (`relay/`).

Out of scope: the model provider, the operating system itself, or machines that are already compromised.

## Safe operation

The full detail is in [`docs/SECURITY.md`](docs/SECURITY.md). In short:

- The model is an untrusted recommender: read every command before approving it.
- Safety lives in local code, not in the prompt.
- The API key is saved only if you agree: in the Windows Credential Manager, or on Linux in `~/.config/tinyshell/credentials` (mode 0600). `tinyshell logout` deletes it.
- Never run it as administrator or root for routine tasks.
- Updates are never automatic and always verify the RSA signature before installing.
