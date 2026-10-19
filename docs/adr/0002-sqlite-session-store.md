# ADR 0002: Saved conversations in SQLite

Status: accepted (2026-10-19).

## Context

A conversation lived only in memory: closing TinyShell or `/reset` lost it. OpenCode keeps
its sessions and lets the user list and resume them; TinyShell users asked for the same.
`AGENTS.md` said "no database", meaning no database server or runtime to install.

Two ways were weighed:

- one append-only file per session (JSON lines), the exe staying near 150 KB;
- SQLite compiled in (the amalgamation): about 600 KB more, transactions, SQL to list and
  search.

On the target machines the size is not felt (2 GB of RAM, hundreds of GB of disk, SQLite's
cache is small). It costs the exe budget, the download (the installer grows from ~190 KB to
~800 KB) and auditability: SQLite is far larger than TinyShell itself. The owner chose
SQLite.

## Decision

- SQLite 3.54.1, the official amalgamation, unmodified, in `vendor/sqlite/` (version, URL
  and SHA3-256 in its README). No network access at build time; Windows, Linux and the
  tests all build the same file.
- Compiled without threads, extensions, UTF-16, JSON, FTS and other unused parts
  (`SQLITE_FLAGS` in `build/sources.sh`). Search uses `LIKE`, which is plenty for one
  user's sessions; FTS5 would add ~140 KB.
- It imports only `KERNEL32` and `msvcrt` on Windows, all functions present on Windows 7.
- Only `src/core/store.c` includes `sqlite3.h` (allowed by `build/checks/includes.sh` for
  that file alone). The store is kept out of `CORE_SRC`, so the core tests, the TUI test
  and fuzzing do not compile SQLite; `tests/test_store.c` does.
- The database lives in the data folder (`%LOCALAPPDATA%\TinyShell` on Windows, readable
  by the user and administrators; `~/.config/tinyshell`, mode 0600, on Linux). It is not
  encrypted: DPAPI per message would rule out searching. API keys are never in it; command
  output may be, so saving can be turned off and old sessions are pruned.
- The exe ceiling in `build/checks/exe-size.sh` rises to make room for it.

## Consequences

- The Windows exe goes from ~144 KB to ~760 KB; the static Linux binary grows similarly.
- Conversations survive closing TinyShell and power cuts (SQLite transactions).
- Updating SQLite is a reviewed copy of two files (see `vendor/sqlite/README.md`).
