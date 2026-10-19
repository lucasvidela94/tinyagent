# SQLite (vendored)

The SQLite amalgamation, unmodified: `sqlite3.c` and `sqlite3.h` from

- version 3.54.1
- https://sqlite.org/2026/sqlite-amalgamation-3540100.zip
- SHA3-256 of the zip: `92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30` (as published on https://sqlite.org/download.html, checked with `openssl dgst -sha3-256`)

SQLite is in the public domain. It is compiled into TinyShell for the saved conversations (`src/core/store.c`); see `docs/adr/0002-sqlite-session-store.md` for why, and `SQLITE_FLAGS` in `build/sources.sh` for the features left out.

To update: download the new amalgamation zip from sqlite.org, check its SHA3-256 against the download page, replace the two files, update the version, URL and hash above, and run `build/check.sh --build` and `build/build-linux-musl.sh`.
