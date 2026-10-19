# Source layout, sourced by the build, test, fuzz and check scripts (one place for paths).
# Layers (enforced by build/checks/includes.sh):
#   src/core              portable, host-tested; includes only core and config.h
#   src/app               the program; includes core, app and the platform interfaces
#   src/platform          the platform seam: one header per OS service
#   src/platform/<os>     adapters for one OS; include core and the platform interfaces
#   vendor/sqlite         SQLite, only for src/core/store.c (docs/adr/0002-sqlite-session-store.md)
# The store is kept out of CORE_SRC so that only the product and tests/test_store.c compile
# SQLite (~10 MB of C); the other tests, the TUI test and fuzzing stay fast.
STORE_SRC=(src/core/store.c)
CORE_SRC=()
for f in src/core/*.c; do
  [[ " ${STORE_SRC[*]} " == *" $f "* ]] || CORE_SRC+=("$f")
done
APP_SRC=(src/app/*.c)
# SQLite is compiled on its own, without our -Werror warning set (it is not our code).
# Left out: threads (TinyShell is single-threaded), extensions, UTF-16, JSON, deprecated
# APIs and other unused parts; FTS too (search uses LIKE).
SQLITE_SRC=vendor/sqlite/sqlite3.c
SQLITE_FLAGS=(-DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_OMIT_LOAD_EXTENSION
  -DSQLITE_OMIT_DEPRECATED -DSQLITE_OMIT_SHARED_CACHE -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_AUTHORIZATION
  -DSQLITE_OMIT_TRACE -DSQLITE_OMIT_COMPLETE -DSQLITE_OMIT_GET_TABLE -DSQLITE_OMIT_TCL_VARIABLE -DSQLITE_OMIT_UTF16
  -DSQLITE_DQS=0 -DSQLITE_LIKE_DOESNT_MATCH_BLOBS -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_JSON
  -DSQLITE_WIN32_NO_ANSI)
WIN32_SRC=(src/platform/win32/*.c)
# POSIX adapters without TLS are tested natively on the host; the ones that need Mbed TLS
# are built with the Linux toolchain (build/build-linux-musl.sh).
POSIX_TLS_SRC=(src/platform/posix/conn_posix.c src/platform/posix/ca_bundle.c src/platform/posix/http_posix.c
  src/platform/posix/verify_posix.c)
POSIX_SRC=()
for f in src/platform/posix/*.c; do
  [[ " ${POSIX_TLS_SRC[*]} " == *" $f "* ]] || POSIX_SRC+=("$f")
done
INCLUDES=(-Isrc -Isrc/core -Isrc/app -Isrc/platform -Ivendor/sqlite)
