#!/usr/bin/env bash
# Docs must not drift from the code: version in the CHANGELOG, every limit in config.h
# named in ARCHITECTURE.md, and the TLS version WinHTTP requests stated in both places.
set -euo pipefail
fail=0
VER=$(grep -E '#define TINYSHELL_VERSION ' src/config.h | sed 's/.*"\(.*\)".*/\1/')
grep -q "^## $VER" CHANGELOG.md || { echo "check-docs: CHANGELOG.md no tiene '## $VER'" >&2; fail=1; }
for sym in MAX_HTTP MAX_HISTORY MAX_OUTPUT MAX_COMMAND MAX_TURNS MAX_TOKENS MAX_DEPTH CMD_TIMEOUT_MS; do
  grep -qE "#define $sym [^ ]" src/config.h src/json.h || { echo "check-docs: $sym no está definido" >&2; fail=1; continue; }
  grep -q "$sym" docs/ARCHITECTURE.md || { echo "check-docs: $sym no aparece en ARCHITECTURE.md" >&2; fail=1; }
done
for f in src/http_win32.c docs/ARCHITECTURE.md; do
  grep -q 'TLS 1.2' "$f" || { echo "check-docs: $f no menciona TLS 1.2" >&2; fail=1; }
done
exit "$fail"
