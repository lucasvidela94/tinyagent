#!/usr/bin/env bash
set -euo pipefail

# Host-side regression tests for the portable core (src/core: util, json, conversation, policy, version, provider, report, stream).
# Needs a host C compiler (gcc or clang); uses AddressSanitizer/UBSan when available.
# On Linux it also tests the POSIX adapters (tests/posix). The Win32 adapters must be
# checked on Windows or under wine: see docs/TEST_PLAN.md.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
. build/sources.sh
CC="${HOST_CC:-cc}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
FLAGS=(-std=c11 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Werror)
if "$CC" -fsanitize=address,undefined -x c -o "$OUT/probe" - <<<'int main(void){return 0;}' 2>/dev/null; then
  FLAGS+=(-fsanitize=address,undefined -fno-sanitize-recover=all)
fi
status=0
# SQLite (not our code, so not our warning set) is built once, only for the store's test,
# and cached by its source, flags and compiler: it takes most of a minute.
CACHE="$ROOT/build/.cache"
key=$( (cat "$SQLITE_SRC"; echo "${SQLITE_FLAGS[*]}"; "$CC" --version) | sha256sum | cut -c1-16)
SQLITE_OBJ="$CACHE/sqlite3-host-$key.o"
if [[ ! -f "$SQLITE_OBJ" ]]; then
  mkdir -p "$CACHE"
  "$CC" -O1 -w "${SQLITE_FLAGS[@]}" -c "$SQLITE_SRC" -o "$SQLITE_OBJ.tmp" && mv "$SQLITE_OBJ.tmp" "$SQLITE_OBJ"
fi
for t in tests/test_*.c; do
  name="$(basename "$t" .c)"
  extra=()
  [[ "$name" == test_store ]] && extra=("${STORE_SRC[@]}" "$SQLITE_OBJ")
  "$CC" "${FLAGS[@]}" "${INCLUDES[@]}" -o "$OUT/$name" "$t" "${CORE_SRC[@]}" "${extra[@]}"
  "$OUT/$name" || status=1
done
# The POSIX adapters run natively on a Linux host, so they are tested here too (with a
# 2 s command timeout, so the timeout path is exercised quickly).
if [[ "$(uname -s)" == Linux ]]; then
  for t in tests/posix/test_*.c; do
    name="$(basename "$t" .c)"
    "$CC" "${FLAGS[@]}" "${INCLUDES[@]}" -Isrc/platform/posix -DCMD_TIMEOUT_MS=2000 -o "$OUT/$name" "$t" "${CORE_SRC[@]}" "${POSIX_SRC[@]}"
    "$OUT/$name" || status=1
  done
fi
exit "$status"
