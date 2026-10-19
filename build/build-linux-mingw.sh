#!/usr/bin/env bash
set -euo pipefail

# Cross-compile from Linux to a 32-bit Windows executable.
# Debian/Ubuntu/Mint: sudo apt install mingw-w64   |   Arch: sudo pacman -S mingw-w64-gcc
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
. build/sources.sh
CC="${CC:-i686-w64-mingw32-gcc}"
if ! command -v "$CC" >/dev/null 2>&1; then
  echo "$CC not found. Install mingw-w64 (see the comment at the top of this script)." >&2
  exit 127
fi
# -flto drops core modules only Linux uses (the PE linker's --gc-sections keeps them).
# Some toolchains (e.g. Arch) default to the UCRT, which Windows 7 lacks unless KB2999226
# is installed. Link against the system msvcrt.dll instead when the toolchain supports it.
CRT=()
PROBE="$(mktemp -d)"
trap 'rm -rf "$PROBE"' EXIT
if "$CC" -mcrtdll=msvcrt-os -x c -o "$PROBE/probe.exe" - <<<'int main(void){return 0;}' 2>/dev/null; then
  CRT=(-mcrtdll=msvcrt-os)
fi
# Icon and version details (src/platform/win32/tinyshell.rc); the numeric version comes from
# TINYSHELL_VERSION ("0.8.0" -> 0,8,0,0).
VER="$(sed -n 's/^#define TINYSHELL_VERSION "\([0-9.]*\)".*/\1/p' src/config.h)"
"${CC%gcc}windres" -Isrc -DTS_VERSION_NUM="${VER//./,},0" -O coff -o "$PROBE/tinyshell.res" src/platform/win32/tinyshell.rc
"$CC" -std=c11 -Os -s -flto -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Werror "${CRT[@]}" "${INCLUDES[@]}" -Isrc/platform/win32 -o tinyshell.exe "${CORE_SRC[@]}" "${APP_SRC[@]}" "${WIN32_SRC[@]}" "$PROBE/tinyshell.res" -lwinhttp -lcrypt32 -ladvapi32 -luser32 -lshell32
file tinyshell.exe 2>/dev/null || true
if command -v "${CC%gcc}objdump" >/dev/null 2>&1; then
  DLLS="$("${CC%gcc}objdump" -p tinyshell.exe | awk '/DLL Name/{print $3}' | tr '\n' ' ')"
  echo "Imported DLLs: $DLLS"
  case "$DLLS" in *api-ms-win-crt*) echo "WARNING: depends on the UCRT; Windows 7 needs KB2999226 for it." >&2 ;; esac
fi
echo "Built $ROOT/tinyshell.exe"
echo "Note: building is not testing on a real Windows 7 x86 machine."
