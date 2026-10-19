#!/usr/bin/env bash
set -euo pipefail

# Static 32-bit Linux build (tinyshell-linux-i386) in the pinned toolchain image
# (build/linux/Dockerfile: GCC + musl for i586, Mbed TLS 4.1). Runs on any x86 Linux from a
# Pentium up, whatever its libc or OpenSSL. Needs Docker. The image is built once (~10 min).
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
. build/sources.sh
IMAGE=tinyshell-linux-build
OUT=tinyshell-linux-i386
docker image inspect "$IMAGE" >/dev/null 2>&1 || docker build -t "$IMAGE" build/linux
SRC=("${CORE_SRC[@]}" "${STORE_SRC[@]}" "${APP_SRC[@]}" "${POSIX_SRC[@]}" "${POSIX_TLS_SRC[@]}")
# SQLite without our -Werror warning set (not our code); see docs/adr/0002-sqlite-session-store.md.
docker run --rm -u "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src "$IMAGE" sh -ec "
  i586-linux-musl-gcc -Os -march=i586 -flto -w ${SQLITE_FLAGS[*]} -c $SQLITE_SRC -o /tmp/sqlite3.o
  i586-linux-musl-gcc -std=c11 -Os -march=i586 -static -s -flto -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
    -Wstrict-prototypes -Werror ${INCLUDES[*]} -Isrc/platform/posix -I/opt/mbedtls/include \
    -o $OUT ${SRC[*]} /tmp/sqlite3.o -L/opt/mbedtls/lib -lmbedtls -lmbedx509 -ltfpsacrypto
  # The CPU floor is i586: nothing newer may run unconditionally (mawk: no \\s in regexes). Only Mbed TLS's AES-NI
  # code has SSE, and it runs after cpuid says the CPU supports it.
  i586-linux-musl-gcc -std=c11 -Os -march=i586 -static -flto ${INCLUDES[*]} -Isrc/platform/posix \
    -I/opt/mbedtls/include -o /tmp/syms ${SRC[*]} /tmp/sqlite3.o -L/opt/mbedtls/lib -lmbedtls -lmbedx509 -ltfpsacrypto
  bad=\$(i586-linux-musl-objdump -d /tmp/syms | awk '/^[0-9a-f]+ <.*>:\$/ {fn=\$2} /[ \\t](cmov|fcomi|fucomi)|xmm/ && fn !~ /aesni/ {print fn}' | sort -u)
  if [ -n \"\$bad\" ]; then echo \"post-i586 instructions in: \$bad\" >&2; exit 1; fi
  i586-linux-musl-readelf -l $OUT | grep -q INTERP && { echo 'the binary is not static' >&2; exit 1; } || true
"
echo "Built $ROOT/$OUT ($(wc -c <"$OUT") bytes): $(./$OUT --version)"
echo "Note: building is not testing on a real old PC."
