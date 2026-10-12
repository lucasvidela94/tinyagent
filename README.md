# TinyShell

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/lucasvidela94/tinyagent)](https://github.com/lucasvidela94/tinyagent/releases/latest)
[![CI](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml/badge.svg)](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml)
[![Windows 7+ 32-bit](https://img.shields.io/badge/Windows-7%2B%2032--bit-blue)](docs/PC_PROFILE.md)
[![Linux i586+](https://img.shields.io/badge/Linux-i586%2B%20static-blue)](docs/adr/0001-linux-port-platform-seam.md)
[![Single exe ~110 KB](https://img.shields.io/badge/exe-%7E110KB-lightgrey)](docs/ARCHITECTURE.md)

A general-purpose terminal agent (in the spirit of OpenCode or Pi) for **old, low-resource PCs**: diagnosing problems, inspecting files, writing scripts, maintenance, or answering questions about the machine.

- One 32-bit binary in C: `tinyshell.exe` for Windows (~110 KB, system DLLs only), and a static Linux build for i586 and newer (in progress). No Node, Python, .NET, services or database.
- The model runs remotely, on any OpenAI-compatible API (default: OpenCode Go); the PC only keeps the console, speaks HTTPS and runs commands.
- **You approve every command** before it runs. Risky ones also ask you to type `EJECUTAR`.
- Main test machine: Noblex NT1013E, Windows 7 SP1 32-bit, Atom N2600, 2 GB RAM.

Site and downloads: **https://tinyshell.lucasan-videla.workers.dev**

> **Status:** prototype. It builds and passes its tests, but it has **not yet run on the real Windows 7 machine**. See [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## En español

TinyShell es un agente de terminal para revivir PCs viejas: vos pedís algo y aprobás cada comando antes de que se ejecute. La app y las instrucciones para usuarios están en español: **instalación, uso y descargas en https://tinyshell.lucasan-videla.workers.dev**. Para reportar un problema, escribí `/reporte` dentro de TinyShell.

## Contents

- [Install](#install-windows) · [Use](#use) · [Provider and model](#provider-and-model)
- [Reporting a problem](#reporting-a-problem) · [Update and roll back](#update-and-roll-back)
- [Security](#security-in-short) · [Building from source](#building-from-source) · [Documentation](#documentation)
- [Contributing](#contributing) · [Support](#support) · [License](#license)

## Install (Windows)

Open **CMD** (not as administrator) and paste the matching line. It installs to `%LOCALAPPDATA%\TinyShell` and adds it to your `PATH`.

**Windows 10 / 11** (they ship `curl`):

```bat
curl -fsSLo "%TEMP%\tinyshell-install.bat" https://github.com/lucasvidela94/tinyagent/releases/latest/download/install.bat && "%TEMP%\tinyshell-install.bat"
```

**Windows 7** (no `curl`; uses `bitsadmin`):

```bat
bitsadmin /transfer tinyshell /download /priority foreground https://github.com/lucasvidela94/tinyagent/releases/latest/download/install.bat "%TEMP%\tinyshell-install.bat" && "%TEMP%\tinyshell-install.bat"
```

`bitsadmin` needs TLS 1.2 enabled system-wide (GitHub requires it), which many Windows 7 machines lack. If it fails, **install from a USB stick**:

1. On another PC, download `tinyshell.exe` and `install.bat` from the [releases page](https://github.com/lucasvidela94/tinyagent/releases/latest).
2. Copy them **together** to a USB stick.
3. On the old PC, run `install.bat` from the stick: it installs the local copy without downloading anything.

The installer shows the executable's SHA-256; compare it with the one in the release.

### Windows 7 requirements

- Service Pack 1 and update **KB3140245** (TinyShell requests TLS 1.2 itself; without the update it cannot connect).
- Up-to-date root certificates and a correct date and time.
- An API key from [OpenCode Go](https://docs.opencode.ai/docs/go/) or another provider (see "Provider and model").

## Use

Log in first (once). The program speaks Spanish:

```text
C:\> tinyshell login
Elegí un proveedor (flechas y Enter):
> opencode-go    OpenCode Go (el modelo se elige después)

API key de opencode-go (clic derecho para pegar): ****************************

Elegí un modelo de opencode-go (38; escribí para filtrar, Enter):
  filtro: kimi_
> kimi-k2.6
  kimi-k2.7-code
  kimi-k3
```

The key is checked against the provider before it is saved (if rejected, nothing is saved) and is stored encrypted in the Windows Credential Manager. The model list is fetched live, so new models show up. `tinyshell logout` deletes the key.

Then open a **new** CMD window and type `tinyshell`. It is a conversation: you ask, the agent proposes commands one step at a time and you approve them, until the task is done.

```text
TinyShell 0.7.0 · opencode-go · /ayuda

 qué programas arrancan con Windows?                    <- your message

$ wmic startup get caption,command                      <- proposal (yellow; red if risky)
  ver programas que inician con Windows
  ejecutar? [s/N] s
  Adobe ARM   "C:\Program Files\...\reader_sl.exe"
  ... +18 líneas · exit=0

Tenés 23 programas al inicio. Los que más pesan son...
─────────────────────────────────────────────────────────
> _
─────────────────────────────────────────────────────────
C:\Users\Pepa · 7.2k in · 0.6k out · 12%      deepseek-v4.1-flash
```

| Command | What it does |
|---|---|
| `/ayuda` | lists the commands |
| `/modelo [x]` | pick a model from the list (or type its name) |
| `/reporte` | report a problem to the developers |
| `/login` | change provider, key or model |
| `/reiniciar` | start a new conversation |
| `/actualizar` | same as `tinyshell --update` |
| `/salir` | quit |

**Ctrl+C** cancels whatever is running (the model's answer or the command) without closing TinyShell; at an empty prompt it quits. The editor has the usual CMD keys: arrows for earlier messages, F7 for history, right-click to paste. Limits that keep an old machine responsive: each command runs without interactive input and is stopped after 180 s; the output the model sees is cut to 24 KB; the history to ~192 KB.

## Provider and model

OpenCode Go is the default. For another OpenAI-compatible service, edit `%LOCALAPPDATA%\TinyShell\config.ini` with Notepad (it is created with examples on first run):

```ini
[provider]
name = openrouter
url = https://openrouter.ai/api/v1/chat/completions
model = deepseek/deepseek-chat
session_header =
```

- `name` identifies the saved API key: each provider has its own.
- `session_header` is only for OpenCode Go (`x-opencode-session`); leave it empty for others.
- A server on your network (LM Studio, Ollama on another PC) works over `http://` without a key; TinyShell warns that the connection is not encrypted.
- `/modelo` lists the provider's models; `/modelo another-model` sets one directly. It is saved to `config.ini`.

## Reporting a problem

If something fails, type `/reporte` inside TinyShell (or `tinyshell reporte` in CMD):

1. describe in one line what happened;
2. TinyShell builds the report (version, OS, model, recent errors and, if you agree, the conversation), saves it under `%LOCALAPPDATA%\TinyShell\reportes\` and **shows everything that would be sent**;
3. only if you answer `s` is it sent: a public GitHub issue gets a summary without personal data, and the full report stays private for the developer.

Without internet, email the saved file to lucasan.videla@gmail.com.

### Telemetry (beta)

While TinyShell is in beta (0.x versions) it sends technical data to find bugs before they are reported: version, OS, model, number of steps and approvals, duration and error codes. **Never** what you type, the commands or their output. It is announced on first run.

```bat
tinyshell telemetria --ver   :: what was sent
tinyshell telemetria off     :: turn it off
```

## Update and roll back

```bat
tinyshell --update     :: checks, asks, verifies the signature and installs
tinyshell --rollback   :: goes back to the previous version
tinyshell --version
tinyshell logout       :: deletes the saved API key
```

It never updates by itself. `--update` works even if the agent is broken (it needs no API key) and:

1. compares your version with the latest release;
2. asks before downloading anything;
3. verifies the executable's **RSA signature** with the public key built into the binary: a release not signed with the project's key is refused, even if it comes from GitHub;
4. runs the new binary with `--version` on **your** machine: if it does not start on that system, it is not installed;
5. keeps the previous one next to it (`tinyshell.old.exe`) for `--rollback`.

If something goes badly wrong, running the installer again (or the USB copy) always leaves a clean install.

## Security, in short

- The model is an untrusted recommender. Read every command in full before approving it.
- Safety lives in local code, not in the prompt: an unknown tool, an empty command, line breaks or more than 2000 bytes are refused before you are asked.
- Risky-command detection is a heuristic, not a barrier: your approval is the real control.
- The API key is saved only if you say yes, in the Windows Credential Manager (on Linux, a file only your user can read). `tinyshell logout` deletes it.
- Do not run it as administrator for routine tasks.
- What you type and the commands' output are sent to the model provider.

Vulnerabilities: email lucasan.videla@gmail.com; do not open a public issue. Details in [`SECURITY.md`](SECURITY.md) and [`docs/SECURITY.md`](docs/SECURITY.md).

## Building from source

On Linux with MinGW-w64 (`sudo apt install mingw-w64` / `sudo pacman -S mingw-w64-gcc`):

```bash
./build/test-core.sh          # host tests (core, and the POSIX adapters on Linux)
./build/build-linux-mingw.sh  # tinyshell.exe (PE32, links msvcrt.dll, not the UCRT)
./build/check.sh --build      # every check plus the Windows build (what CI runs)
./build/fuzz.sh               # fuzzes the JSON, SSE and HTTP parsers (clang with libFuzzer)
./build/build-linux-musl.sh   # static i586 Linux binary (needs Docker)
```

`pnpm install` once enables the pre-commit hook (husky), which runs `build/check.sh`. Node is a development tool only: TinyShell does not need it to run. The code is layered (`src/core`, `src/app`, `src/platform`); see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

On Windows, from Visual Studio's **x86 Native Tools Command Prompt**: `build\build-windows-msvc.bat`.

A binary you build yourself does not carry the release key unless you generate your own, so `--update` will not work with it; everything else does.

## Publishing a release (maintainer)

Once: `./build/gen-signing-key.sh` creates the private key in `~/.config/tinyshell/` (keep an offline backup; it never goes into the repo) and writes the public half to `keys/` and `src/release_key.h` (those are committed).

For each release, from a clean `main`:

1. Bump `TINYSHELL_VERSION` in `src/config.h` and add `## X.Y.Z` to `CHANGELOG.md`; commit.
2. `./build/release.sh X.Y.Z --dry-run` to review the artifacts in `dist/`.
3. `./build/release.sh X.Y.Z`: runs the tests, builds, signs, tags and publishes with `gh`.

See [`CHANGELOG.md`](CHANGELOG.md) for the history.

## Contributing

Every contribution counts. There is no formal code of conduct: if you contribute in good faith, you are welcome.

1. Read [`CONTRIBUTING.md`](CONTRIBUTING.md) (5 minutes).
2. For bugs use the [bug report form](.github/ISSUE_TEMPLATE/bug_report.yml); for ideas, the [feature request form](.github/ISSUE_TEMPLATE/feature_request.yml).
3. A small pull request with tests goes in fast. The [pull request template](.github/PULL_REQUEST_TEMPLATE.md) guides you.

## Support

- Usage question? [Discussions](https://github.com/lucasvidela94/tinyagent/discussions).
- Bug? [Bug report](https://github.com/lucasvidela94/tinyagent/issues/new/choose).
- No internet: run `scripts/collect-diagnostics.bat` and email the result to lucasan.videla@gmail.com.
- Details in [`SUPPORT.md`](SUPPORT.md).

## Documentation

- [`docs/GOALS.md`](docs/GOALS.md): goal and definition of done.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): layers, modules and data flow.
- [`docs/adr/`](docs/adr/): architecture decisions (the Linux port).
- [`GLOSSARY.md`](GLOSSARY.md): domain terms.
- [`docs/SYSTEM_PROMPT.md`](docs/SYSTEM_PROMPT.md): how the system prompt is written.
- [`docs/ACCESS_POINTS.md`](docs/ACCESS_POINTS.md): local entry points, network and commands.
- [`docs/SECURITY.md`](docs/SECURITY.md): trust model and limits.
- [`docs/OPERATIONS.md`](docs/OPERATIONS.md): installation, TLS problems, diagnostics.
- [`docs/API.md`](docs/API.md): the contract with the model provider.
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) and [`docs/ROADMAP.md`](docs/ROADMAP.md).
- [`docs/PC_PROFILE.md`](docs/PC_PROFILE.md): the test machine.
- [`scripts/collect-diagnostics.bat`](scripts/collect-diagnostics.bat): local report without the API.

## License

[MIT](LICENSE).
