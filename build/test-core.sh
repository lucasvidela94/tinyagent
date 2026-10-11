#!/usr/bin/env bash
set -euo pipefail

# Host-side regression tests for the portable modules in src/ (util, json, conversation, policy, version, provider, report, stream).
# Needs a host C compiler (gcc or clang); uses AddressSanitizer/UBSan when available.
# The Win32 adapters (*_win32.c, main.c) must be checked on Windows: see docs/TEST_PLAN.md.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CC="${HOST_CC:-cc}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
FLAGS=(-std=c99 -g -Wall -Wextra -Werror)
if "$CC" -fsanitize=address,undefined -x c -o "$OUT/probe" - <<<'int main(void){return 0;}' 2>/dev/null; then
  FLAGS+=(-fsanitize=address,undefined -fno-sanitize-recover=all)
fi
CORE=(src/util.c src/json.c src/conversation.c src/policy.c src/version.c src/provider.c src/report.c src/stream.c)
status=0
for t in tests/test_*.c; do
  name="$(basename "$t" .c)"
  "$CC" "${FLAGS[@]}" -o "$OUT/$name" "$t" "${CORE[@]}"
  "$OUT/$name" || status=1
done
exit "$status"
