#!/usr/bin/env bash
set -euo pipefail

# Builds dist/TinyShell-Setup.exe (the Windows installer) from tinyshell.exe with NSIS, in a
# small Docker image (build/installer/Dockerfile). Build the exe first.
#   build/build-installer.sh [path/to/tinyshell.exe] [output.exe]
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
EXE="${1:-tinyshell.exe}"
OUT="${2:-dist/TinyShell-Setup.exe}"
IMAGE=tinyshell-nsis
VER="$(sed -n 's/^#define TINYSHELL_VERSION "\([0-9.]*\)".*/\1/p' src/config.h)"
[ -f "$EXE" ] || { echo "build-installer: $EXE not found (build it first)" >&2; exit 1; }
mkdir -p "$(dirname "$OUT")"
docker image inspect "$IMAGE" >/dev/null 2>&1 || docker build -q -t "$IMAGE" build/installer >/dev/null
docker run --rm -u "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src/build/installer "$IMAGE" \
  makensis -V2 -DVERSION="$VER" -DEXE="/src/$EXE" -DOUTFILE="/src/$OUT" tinyshell.nsi
echo "Built $OUT ($(wc -c <"$OUT") bytes) for TinyShell $VER"
