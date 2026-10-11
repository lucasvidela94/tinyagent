#!/usr/bin/env bash
set -euo pipefail

# Short fuzz run of the parsers that read remote input (JSON, SSE). Needs clang with libFuzzer.
#   build/fuzz.sh [seconds_per_target=20]
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
. build/sources.sh
SECS="${1:-20}"
CC="${FUZZ_CC:-clang}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
FLAGS=(-std=c11 -g "${INCLUDES[@]}" -fsanitize=fuzzer,address,undefined)
# libFuzzer brings its own main(), so the probe defines only the fuzz entry point.
printf '#include <stddef.h>\n#include <stdint.h>\nint LLVMFuzzerTestOneInput(const uint8_t *d, size_t n) { (void)d; (void)n; return 0; }\n' >"$OUT/probe.c"
if ! "$CC" "${FLAGS[@]}" -o "$OUT/probe" "$OUT/probe.c" 2>/dev/null; then
  echo "fuzz: $CC sin libFuzzer; no se corrió nada" >&2
  exit 1
fi
mkdir -p "$OUT/json" "$OUT/stream"
printf '{"choices":[{"message":{"content":"hola"}}]}' >"$OUT/json/seed"
printf 'data: {"choices":[{"delta":{"content":"hi"}}]}\n\ndata: [DONE]\n\n' >"$OUT/stream/seed"
mkdir -p fuzz-findings # a crashing input is kept here (git-ignored) to become a regression test
for t in json stream; do
  "$CC" "${FLAGS[@]}" -o "$OUT/fuzz_$t" "tests/fuzz/$t.c" "${CORE_SRC[@]}"
  "$OUT/fuzz_$t" -max_total_time="$SECS" -max_len=65536 -artifact_prefix=fuzz-findings/ "$OUT/$t" 2>&1 | tail -1
done
echo "fuzz: OK ($SECS s por parser)"
