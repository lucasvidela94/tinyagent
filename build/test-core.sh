#!/usr/bin/env bash
set -euo pipefail

# Host-side regression tests for the portable core (src/core: util, json, conversation, policy, version, provider, report, stream).
# Needs a host C compiler (gcc or clang); uses AddressSanitizer/UBSan when available.
# The Win32 adapters (*_win32.c, main.c) must be checked on Windows: see docs/TEST_PLAN.md.
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
for t in tests/test_*.c; do
  name="$(basename "$t" .c)"
  "$CC" "${FLAGS[@]}" "${INCLUDES[@]}" -o "$OUT/$name" "$t" "${CORE_SRC[@]}"
  "$OUT/$name" || status=1
done
exit "$status"
