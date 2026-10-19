# TinyShell

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/lucasvidela94/tinyagent)](https://github.com/lucasvidela94/tinyagent/releases/latest)
[![CI](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml/badge.svg)](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml)
[![Windows 7+ 32-bit](https://img.shields.io/badge/Windows-7%2B%2032--bit-blue)](docs/PC_PROFILE.md)
[![Linux i586+](https://img.shields.io/badge/Linux-i586%2B%20static-blue)](docs/adr/0001-linux-port-platform-seam.md)
[![Single exe ~130 KB](https://img.shields.io/badge/exe-%7E130KB-lightgrey)](docs/ARCHITECTURE.md)

A general-purpose terminal agent (in the spirit of OpenCode or Pi) for **old, low-resource PCs**: diagnosing problems, inspecting files, writing scripts, maintenance, or answering questions about the machine.

- One 32-bit binary per platform, in C: `tinyshell.exe` for Windows 7 and later (~750 KB, system DLLs only) and `tinyshell-linux-i386` for any x86 Linux from a Pentium up (static, with its own TLS). No Node, Python, .NET, services or database server: conversations are saved with SQLite compiled in, and `/sessions` resumes them.
- The model runs remotely, on any OpenAI-compatible API (default: OpenCode Go); the PC only keeps the console, speaks HTTPS and runs commands.
- **You approve every command** before it runs: once, always for that kind of command during the session, or no, telling the model what to do instead.
- The interface speaks English or Spanish, following the system's language.
- Main test machine: Noblex NT1013E, Windows 7 SP1 32-bit, Atom N2600, 2 GB RAM.

Site and downloads (English and Spanish): **https://tinyshell.lucasan-videla.workers.dev**

> **Status:** prototype. It builds and passes its tests, and it was checked under wine and in old Linux userlands (Docker), but it has **not yet run on the real Windows 7 machine or on old Linux hardware**. See [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## Contents

- [Install on Windows](#install-windows) · [Install on Linux](#install-linux) · [Use](#use) · [Language](#language) · [Provider and model](#provider-and-model)
- [Reporting a problem](#reporting-a-problem) · [Update and roll back](#update-and-roll-back)
- [Security](#security-in-short) · [Building from source](#building-from-source) · [Documentation](#documentation)
- [Contributing](#contributing) · [Support](#support) · [License](#license)

## Install (Windows)

1. Download **[TinyShell-Setup.exe](https://github.com/lucasvidela94/tinyagent/releases/latest/download/TinyShell-Setup.exe)** (also on the site and the [releases page](https://github.com/lucasvidela94/tinyagent/releases/latest)).
2. Run it and follow the steps. It installs for your user only, **no administrator rights**, into `%LOCALAPPDATA%\TinyShell`, adds a Start menu entry and puts `tinyshell` on your `PATH`.
3. Leave **"Set it up now"** checked at the end: it runs `tinyshell login`.

The installer is not code-signed yet, so Windows may warn that the publisher is unknown (on Windows 10/11: "More info", then "Run anyway"). Its SHA-256 is in the release notes and in `SHA256SUMS`; check it with `certutil -hashfile TinyShell-Setup.exe SHA256`. Uninstall it from **Programs and Features** like any other program.

From CMD, in one line (it downloads the installer and runs it):

```bat
curl -fsSLo "%TEMP%\TinyShell-Setup.exe" https://github.com/lucasvidela94/tinyagent/releases/latest/download/TinyShell-Setup.exe && "%TEMP%\TinyShell-Setup.exe"
```

On Windows 7, which has no `curl`, use `bitsadmin`:

```bat
bitsadmin /transfer tinyshell /download /priority foreground https://github.com/lucasvidela94/tinyagent/releases/latest/download/TinyShell-Setup.exe "%TEMP%\TinyShell-Setup.exe" && "%TEMP%\TinyShell-Setup.exe"
```

`bitsadmin` and a browser need TLS 1.2 enabled system-wide (GitHub requires it), which many Windows 7 machines lack. Then **use a USB stick**: download `TinyShell-Setup.exe` on another PC, copy it to the stick and run it on the old one.

Prefer no installer? `tinyshell.exe` from the release also runs on its own (portable).

### Windows 7 requirements

- Service Pack 1 and update **KB3140245** (TinyShell requests TLS 1.2 itself; without the update it cannot connect).
- Up-to-date root certificates and a correct date and time.

## Install (Linux)

One static 32-bit binary for x86 PCs from the Pentium up (i586, no SSE2 needed). It runs on 32- and 64-bit distros whatever their libc or OpenSSL, and brings its own TLS and root certificates, so old distros with expired certificates still connect.

```sh
curl -fsSLo /tmp/tinyshell-install.sh https://github.com/lucasvidela94/tinyagent/releases/latest/download/install.sh && sh /tmp/tinyshell-install.sh
```

(`wget -qO /tmp/tinyshell-install.sh <same URL>` works too.) It checks the download against `SHA256SUMS`, installs `~/.local/bin/tinyshell`, checks that it starts and tells you if that folder is not on your `PATH`. Then run `tinyshell login`.

Very old distros may not reach GitHub at all (their `curl`/`wget` predate TLS 1.2). Download `tinyshell-linux-i386` and `install.sh` on another PC, copy them to a USB stick, and run `sh install.sh ./tinyshell-linux-i386`. Once installed, `tinyshell --update` uses TinyShell's own TLS.

Settings and the saved API key (mode 0600, only if you agree) live in `~/.config/tinyshell/`.

## Use

You need an API key from [OpenCode Go](https://docs.opencode.ai/docs/go/) or another provider (see "Provider and model"). Log in first (once), in CMD on Windows or a terminal on Linux:

```text
C:\> tinyshell login
Pick a provider (arrows and Enter):
> opencode-go    OpenCode Go (pick the model next)

API key for opencode-go (Ctrl+V or right-click to paste): ****************************

Pick a model from opencode-go (38; type to filter, Enter):
  filter: kimi_
> kimi-k2.6
  kimi-k2.7-code
  kimi-k3
```

The key is checked against the provider before it is saved (if rejected, nothing is saved). It is stored encrypted in the Windows Credential Manager, or on Linux in `~/.config/tinyshell/credentials`, readable only by your user. The model list is fetched live, so new models show up. `tinyshell logout` deletes the key.

Then start TinyShell: from the Start menu or by typing `tinyshell` in a **new** CMD window on Windows, or `tinyshell` in a terminal on Linux. It is a conversation: you ask, the agent proposes commands one step at a time and you approve them, until the task is done. The answer streams in as the model writes it.

```text
TinyShell 0.11.0 · opencode-go · /help

 which programs start with Windows?                     <- your message

  $ wmic startup get caption,command                    <- the command, in a shaded panel
  list the programs that start with Windows
Run it?                                                 <- menu (arrows + Enter),
> Yes, once                                                erased once you choose
  Yes, always "wmic startup" in this session
  No, and tell it what to do instead

  $ wmic startup get caption,command                    <- after "Yes, once": the output
  list the programs that start with Windows                in the same panel
  (15 earlier lines)
  Adobe ARM   "C:\Program Files\...\reader_sl.exe"
  exit=0

You have 23 startup programs. The heaviest ones are...
─────────────────────────────────────────────────────────
> _
─────────────────────────────────────────────────────────
C:\Users\User · 7.2k in · 0.6k out · 12%       deepseek-v4.1-flash
```

The footer shows the current folder, the tokens sent and received, how full the conversation history is, and the model.

| Command | What it does |
|---|---|
| `/help` | lists the commands |
| `/model [x]` | pick a model from the list (or type its name) |
| `/login` | change provider, key or model |
| `/report` | report a problem to the developers |
| `/reset` | start a new conversation |
| `/update` | same as `tinyshell --update` |
| `/quit` | quit |

**Ctrl+C** cancels whatever is running (the model's answer or the command) without closing TinyShell; at an empty prompt it quits. On Windows the editor has the usual CMD keys: arrows for earlier messages, F7 for history, right-click to paste; on Linux, the terminal's own line editing. Commands run in `cmd.exe` on Windows and `/bin/sh` on Linux. Limits that keep an old machine responsive: each command runs without interactive input and is stopped after 180 s; the output the model sees is cut to 24 KB; the history to ~192 KB.

## Language

The interface is English or Spanish, following the system's language (Windows' display language; `LANG` on Linux). To choose it yourself, set `language` in the `[ui]` section of `config.ini` to `en`, `es` or `auto`. Whatever language you write in, the model answers in it.

## Provider and model

OpenCode Go is the default. For another OpenAI-compatible service, edit `config.ini` (created with examples on first run): `%LOCALAPPDATA%\TinyShell\config.ini` on Windows (Notepad is fine), `~/.config/tinyshell/config.ini` on Linux.

```ini
[provider]
name = openrouter
url = https://openrouter.ai/api/v1/chat/completions
model = deepseek/deepseek-chat
session_header =

[ui]
language = auto
```

- `name` identifies the saved API key: each provider has its own.
- `session_header` is only for OpenCode Go (`x-opencode-session`); leave it empty for others.
- A server on your network (LM Studio, Ollama on another PC) works over `http://` without a key; TinyShell warns that the connection is not encrypted.
- `/model` lists the provider's models; `/model another-model` sets one directly. It is saved to `config.ini`.

## Reporting a problem

If something fails, type `/report` inside TinyShell (or `tinyshell report` in a terminal):

1. describe in one line what happened;
2. TinyShell builds the report (version, OS, model, recent errors and, if you agree, the conversation), saves it in the `reports` folder next to `config.ini` and **shows everything that would be sent**;
3. only if you answer yes is it sent: a public GitHub issue gets a summary without personal data, and the full report stays private for the developer.

Without internet, email the saved file to lucasan.videla@gmail.com.

### Telemetry (beta)

While TinyShell is in beta (0.x versions) it sends technical data to find bugs before they are reported: version, OS, model, number of steps and approvals, duration and error codes. **Never** what you type, the commands or their output. It is announced on first run.

```text
tinyshell telemetry --show    what was sent
tinyshell telemetry off       turn it off
```

## Update and roll back

```text
tinyshell --update      checks, asks, verifies the signature and installs
tinyshell --rollback    goes back to the previous version
tinyshell --version     shows the version
tinyshell logout        deletes the saved API key
```

It never updates by itself. `--update` works even if the agent is broken (it needs no API key) and:

1. compares your version with the latest release;
2. asks before downloading anything;
3. verifies the executable's **RSA signature** with the public key built into the binary: a release not signed with the project's key is refused, even if it comes from GitHub;
4. runs the new binary with `--version` on **your** machine: if it does not start on that system, it is not installed;
5. keeps the previous one next to it (`tinyshell.old.exe` on Windows, `tinyshell.old` on Linux) for `--rollback`.

If something goes badly wrong, installing again (the Windows installer or `install.sh`, also from a USB copy) always leaves a clean install.

## Security, in short

- The model is an untrusted recommender. Read every command in full before approving it.
- Safety lives in local code, not in the prompt: an unknown tool, an empty command, line breaks or more than 2000 bytes are refused before you are asked.
- Risky-command detection is a heuristic, not a barrier: your approval is the real control.
- The API key is saved only if you say yes, in the Windows Credential Manager (on Linux, a file only your user can read). `tinyshell logout` deletes it.
- Do not run it as administrator (or root) for routine tasks: commands run with TinyShell's permissions.
- What you type and the commands' output are sent to the model provider.

Security vulnerabilities (a way to run commands without approval, bypass the update signature or steal API keys): email lucasan.videla@gmail.com instead of opening a public issue, so it can be fixed before it is public. For anything else, issues are welcome. Details in [`SECURITY.md`](SECURITY.md) and [`docs/SECURITY.md`](docs/SECURITY.md).

## Building from source

On Linux with MinGW-w64 (`sudo apt install mingw-w64` / `sudo pacman -S mingw-w64-gcc`):

```bash
./build/test-core.sh          # host tests (core, and the POSIX adapters on Linux)
./build/build-linux-mingw.sh  # tinyshell.exe (PE32, links msvcrt.dll, not the UCRT)
./build/check.sh --build      # every check plus the Windows build (what CI runs)
./build/fuzz.sh               # fuzzes the JSON, SSE and HTTP parsers (clang with libFuzzer)
./build/build-linux-musl.sh   # static i586 Linux binary (needs Docker)
```

`pnpm install` once enables the pre-commit hook (husky), which runs `build/check.sh`. Node is a development tool only: TinyShell does not need it to run. The code is layered (`src/core`, `src/app`, `src/platform`); see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md). New user-facing strings go through `tr()` with a Spanish entry in `src/core/i18n_es.c` (`build/checks/i18n.sh` checks it).

On Windows, from Visual Studio's **x86 Native Tools Command Prompt**: `build\build-windows-msvc.bat`.

A binary you build yourself does not carry the release key unless you generate your own, so `--update` will not work with it; everything else does.

## Publishing a release (maintainer)

Once: `./build/gen-signing-key.sh` creates the private key in `~/.config/tinyshell/` (keep an offline backup; it never goes into the repo) and writes the public half to `keys/` and `src/release_key.h` (those are committed).

For each release, from a clean `main`:

1. Bump `TINYSHELL_VERSION` in `src/config.h` and add `## X.Y.Z` to `CHANGELOG.md`; commit.
2. `./build/release.sh X.Y.Z --dry-run` to review the artifacts in `dist/`.
3. `./build/release.sh X.Y.Z`: runs the tests, builds `tinyshell.exe`, `TinyShell-Setup.exe` and `tinyshell-linux-i386` (needs mingw-w64 and Docker), signs both binaries, tags and publishes with `gh`.

See [`CHANGELOG.md`](CHANGELOG.md) for the history.

## Contributing

Every contribution counts. There is no formal code of conduct: if you contribute in good faith, you are welcome.

1. Read [`CONTRIBUTING.md`](CONTRIBUTING.md) (5 minutes).
2. For bugs use the [bug report form](.github/ISSUE_TEMPLATE/bug_report.yml); for ideas, the [feature request form](.github/ISSUE_TEMPLATE/feature_request.yml).
3. A small pull request with tests goes in fast. The [pull request template](.github/PULL_REQUEST_TEMPLATE.md) guides you.

## Support

- Usage question? [Discussions](https://github.com/lucasvidela94/tinyagent/discussions).
- Bug? [Bug report](https://github.com/lucasvidela94/tinyagent/issues/new/choose).
- No internet: email the report TinyShell saved (see "Reporting a problem") to lucasan.videla@gmail.com. On Windows, `scripts/collect-diagnostics.bat` also gathers system details without the API.
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
- [`docs/PC_PROFILE.md`](docs/PC_PROFILE.md): the Windows test machine.
- [`scripts/collect-diagnostics.bat`](scripts/collect-diagnostics.bat): local Windows report without the API.

## License

[MIT](LICENSE).
