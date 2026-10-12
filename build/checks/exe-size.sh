#!/usr/bin/env bash
# Size budget for the Windows exe: one binary, a checkable size.
# Now ~110 KB (2026-10-11). Ceiling 128 KB; raising it needs a reason in the commit.
set -euo pipefail
MAX="${MAX_EXE:-131072}"
f="${1:-tinyshell.exe}"
[ -f "$f" ] || { echo "check-exe-size: $f does not exist (build it first)" >&2; exit 1; }
n=$(wc -c <"$f")
if [ "$n" -gt "$MAX" ]; then
  echo "check-exe-size: $f is $n bytes (ceiling $MAX). Trim it or justify raising the ceiling." >&2
  exit 1
fi
echo "check-exe-size: $f $n bytes (ceiling $MAX) OK"
