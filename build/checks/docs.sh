#!/usr/bin/env bash
# Docs must not drift from the code: version in the CHANGELOG, every limit in config.h
# named in ARCHITECTURE.md, the TLS version WinHTTP requests stated in both places, and
# every source path the docs name still existing.
set -euo pipefail
fail=0
VER=$(grep -E '#define TINYSHELL_VERSION ' src/config.h | sed 's/.*"\(.*\)".*/\1/')
grep -q "^## $VER" CHANGELOG.md || { echo "check-docs: CHANGELOG.md no tiene '## $VER'" >&2; fail=1; }
for sym in MAX_HTTP MAX_HISTORY MAX_OUTPUT MAX_COMMAND MAX_TURNS MAX_TOKENS MAX_DEPTH CMD_TIMEOUT_MS; do
  grep -qE "#define $sym [^ ]" src/config.h src/core/json.h || { echo "check-docs: $sym no está definido" >&2; fail=1; continue; }
  grep -q "$sym" docs/ARCHITECTURE.md || { echo "check-docs: $sym no aparece en ARCHITECTURE.md" >&2; fail=1; }
done
for f in src/platform/win32/http_win32.c docs/ARCHITECTURE.md; do
  grep -q 'TLS 1.2' "$f" || { echo "check-docs: $f no menciona TLS 1.2" >&2; fail=1; }
done
# Every source path a doc names must exist (the CHANGELOG is history and keeps old paths).
while IFS=: read -r doc path; do
  [ -e "$path" ] || { echo "check-docs: $doc nombra $path, que no existe" >&2; fail=1; }
done < <(grep -oE 'src/[a-zA-Z0-9_/]+\.(c|h)' -r --include='*.md' . --exclude=CHANGELOG.md --exclude-dir=node_modules | sort -u)
exit "$fail"
