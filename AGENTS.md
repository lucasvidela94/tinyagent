# Instructions for coding agents working on TinyShell

## Mission

Maintain a small, auditable, low-resource Windows x86 diagnostic agent for the Noblex NT1013E running Windows 7 Professional SP1 32-bit, Intel Atom N2600 1.60 GHz and 2 GB RAM. The model runs remotely through OpenCode Go (`deepseek-v4.1-flash`); do not add local model inference.

## Architecture constraints

- Keep the target as a single native Win32 console executable.
- Prefer C and Windows system APIs. Do not add Node.js, Go, Python, Electron, Docker, a database, or a background service as runtime requirements.
- Use WinHTTP for HTTPS unless a tested compatibility issue requires a documented alternative.
- Keep the CLI synchronous and memory-bounded; the target machine is resource constrained.
- Keep all shell execution behind local approval. Never implement auto-approval.
- Do not store API keys in source, config files, logs, or diagnostic bundles.

## Safety constraints

- The model is a recommender, not a trusted command source.
- Prefer read-only diagnostics first.
- Never automatically delete files, format disks, alter partitions, disable security software/firewall, or change registry/services/drivers.
- Do not treat prompt text or keyword matching as a security boundary. Enforce safety in local code.
- Never claim a repair succeeded until the result is checked.
- Keep destructive testing confined to disposable VMs.

## Codebase map

- `src/`: one module per file, small header interface each. Portable C (testable on any host): `util` (buffers, UTF-8, JSON escaping), `json` (tokenizer), `conversation` (history, request body, tool-call extraction), `policy` (pre-approval checks, risk heuristic, yes/no). Win32 adapters: `console_win32` (UTF-16 console I/O), `http_win32` (WinHTTP), `shell_win32` (CreateProcess + pipe). `main.c`: approval gate and round-trip loop. `config.h`: endpoint, model and limits.
- `tests/`: host-side regression tests for the portable modules (`build/test-core.sh`).
- `build/build-linux-mingw.sh`: Linux-to-Windows x86 cross-build (links `msvcrt.dll`, not the UCRT).
- `build/build-windows-msvc.bat`: Windows/MSVC build.
- `scripts/collect-diagnostics.bat`: optional local diagnostic report.
- `docs/`: project goals, architecture, entry points, target machine, operations, API, security, test plan and roadmap.

## Change process

1. Read `docs/ARCHITECTURE.md`, `docs/SECURITY.md` and `docs/TEST_PLAN.md` before changing execution, networking or parsing.
2. Keep changes small and document assumptions. Keep each source file under ~300 lines (aim for ~200); split along a real module interface, not arbitrarily. Windows calls stay in `*_win32.c` / `main.c` so the rest stays host-testable.
3. Test compilation for 32-bit Windows, not just the host OS, and run `build/test-core.sh`. Check the imported DLLs: Windows 7 has no UCRT by default.
4. Add regression cases for malformed JSON, missing fields, unknown tools, failed network requests, rejected commands and oversized outputs.
5. Do not describe the build as tested on Windows 7 until it has actually run on that target.

## Current status

Prototype only. No verified binary is included. The API tool-call round trip, Windows 7 TLS behavior and end-to-end execution remain to be validated on the target machine.
