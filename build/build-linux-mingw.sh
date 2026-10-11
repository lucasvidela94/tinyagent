#!/usr/bin/env bash
set -euo pipefail

# Cross-compile from Linux to a 32-bit Windows executable.
# Debian/Ubuntu/Mint: sudo apt install mingw-w64
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CC="${CC:-i686-w64-mingw32-gcc}"
if ! command -v "$CC" >/dev/null 2>&1; then
  echo "No encuentro $CC. En Debian/Ubuntu/Mint instalá: sudo apt install mingw-w64" >&2
  exit 127
fi
"$CC" -std=c99 -Os -s -Wall -Wextra -Wno-unused-parameter \
  -o tinyshell.exe agent.c -lwinhttp
file tinyshell.exe 2>/dev/null || true
echo "Generado: $ROOT/tinyshell.exe"
echo "Nota: compilar no sustituye las pruebas reales en Windows 7 x86."
