#!/usr/bin/env bash
set -euo pipefail

# The real TUI on the POSIX console adapter, driven in a pseudo-terminal: tests/tui/demo.c
# scripted by tests/tui/run.py, which checks the screen with pyte (UTF-8 and LANG=C).
# Needs a C compiler and pyte (uses `uv run --with pyte` when uv is installed).
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
. build/sources.sh
CC="${HOST_CC:-cc}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
"$CC" -std=c11 -Wall -Wextra -Werror "${INCLUDES[@]}" -Isrc/platform/posix -o "$OUT/demo" tests/tui/demo.c \
  src/app/tui.c src/app/tui_select.c src/app/question.c "${CORE_SRC[@]}" src/platform/posix/console_posix.c \
  src/platform/posix/console_input_posix.c src/platform/posix/cancel_posix.c
if command -v uv >/dev/null 2>&1; then
  uv run -q --with pyte python3 tests/tui/run.py "$OUT/demo"
else
  python3 tests/tui/run.py "$OUT/demo"
fi
