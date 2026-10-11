#!/usr/bin/env bash
set -euo pipefail

# Cross-compile from Linux to a 32-bit Windows executable.
# Debian/Ubuntu/Mint: sudo apt install mingw-w64   |   Arch: sudo pacman -S mingw-w64-gcc
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CC="${CC:-i686-w64-mingw32-gcc}"
if ! command -v "$CC" >/dev/null 2>&1; then
  echo "No encuentro $CC. Instalá mingw-w64 (ver comentario al inicio del script)." >&2
  exit 127
fi
# Some toolchains (e.g. Arch) default to the UCRT, which Windows 7 lacks unless KB2999226
# is installed. Link against the system msvcrt.dll instead when the toolchain supports it.
CRT=()
PROBE="$(mktemp -d)"
trap 'rm -rf "$PROBE"' EXIT
if "$CC" -mcrtdll=msvcrt-os -x c -o "$PROBE/probe.exe" - <<<'int main(void){return 0;}' 2>/dev/null; then
  CRT=(-mcrtdll=msvcrt-os)
fi
"$CC" -std=c11 -Os -s -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Werror "${CRT[@]}" -o tinyshell.exe src/*.c -lwinhttp -lcrypt32 -ladvapi32
file tinyshell.exe 2>/dev/null || true
if command -v "${CC%gcc}objdump" >/dev/null 2>&1; then
  DLLS="$("${CC%gcc}objdump" -p tinyshell.exe | awk '/DLL Name/{print $3}' | tr '\n' ' ')"
  echo "DLLs importadas: $DLLS"
  case "$DLLS" in *api-ms-win-crt*) echo "AVISO: depende de la UCRT; en Windows 7 requiere KB2999226." >&2 ;; esac
fi
echo "Generado: $ROOT/tinyshell.exe"
echo "Nota: compilar no sustituye las pruebas reales en Windows 7 x86."
