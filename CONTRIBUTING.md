# Contributing to TinyShell

Thanks for wanting to help. There is no bureaucracy: if an idea makes the project better, it is welcome.

## Ways to help

- Test on a real old PC (Windows 7 32-bit, or an old 32-bit Linux distro) and say what happened.
- Report bugs with steps to reproduce them.
- Propose or send a small fix.
- Improve the docs, the messages or the installer.
- Add tests in `tests/` for cases that are not covered yet.

No need to ask first: open the issue or the pull request directly.

## Before writing code

1. Read `docs/ARCHITECTURE.md`, `docs/SECURITY.md` and `docs/TEST_PLAN.md` if you will touch execution, networking or parsing.
2. Read `AGENTS.md` (the repo's rules) and `GLOSSARY.md` (its terms).
3. For a large change, open a short issue with the idea first, so no work is wasted.

## Getting started

```bash
# host tests (the portable core, and the POSIX adapters on Linux)
./build/test-core.sh

# Windows 32-bit cross-build (needs mingw-w64)
./build/build-linux-mingw.sh

# static i586 Linux build (needs Docker)
./build/build-linux-musl.sh

# everything CI runs
./build/check.sh --build
```

On Windows, from the **x86 Native Tools Command Prompt**: `build\build-windows-msvc.bat`.

`pnpm install` once enables the pre-commit hook (husky), which runs `build/check.sh`. Node is a development tool only, never a runtime requirement.

## Code rules

- One commit does one thing, under ~400 lines not counting tests.
- Source files stay around ~200 lines (~300 at most). A growing file is split along a real module interface.
- Operating system calls live in `src/platform/win32/` and `src/platform/posix/`; `core` and `app` never include an adapter (`build/checks/includes.sh` enforces it).
- No new dependency without a reason in the commit message.
- No single-implementation abstractions (no vtables or providers until there are two real cases).
- A behavior change needs a test. A refactor changes no behavior.
- Comments explain *why*; they never repeat the code.
- `gets/sprintf/strtok/system/popen/wsprintf/A-variants/alloca` are banned. `strcpy/strcat` only with a length guard on the lines above (`build/checks/banned.sh` checks it).
- Never edit `build/` or the config to make a check pass: fix the code or discuss the check.
- Repo text (code, comments, docs, commits) is English; only what end users see (the program's strings, `install.bat`, the site) is Spanish.

## Commits and pull requests

- Short commit subjects in English, imperative mood: `Add a timeout to http_get`, not `Added...`.
- Explain *why* the change is needed and what you tested (command and result).
- One pull request, one topic. Keep it small and easy to review.
- The pull request template asks what, why and how, the related issue, tests and the impact on docs or releases. Fill it in rather than deleting it.
- CI must pass: static checks, tests with sanitizers, the Win32 cross-build, the exe size budget, the TUI test and fuzzing.

## Expected tests

- Cases for malformed JSON, missing fields, unknown tools, a failed network, rejected commands and huge outputs.
- If you touch `json`, `stream` or `httpwire`, run `./build/fuzz.sh`.
- Do not say something works on Windows 7 (or an old distro) until it ran there; say so when it was tested under wine or Docker.

## Reporting bugs

Use the bug report form: what you expected, what happened, the version (`tinyshell --version`), the OS, provider and model, and minimal steps to reproduce. For a crash with security impact, see `SECURITY.md`.

## Questions

Open an issue or a discussion. There are no silly questions here.
