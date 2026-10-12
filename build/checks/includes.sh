#!/usr/bin/env bash
# Includes say what a file depends on, so they are checked, not promised:
#   - system headers only from the allowlist (no third-party libraries): C99 anywhere, OS
#     headers only in that OS's adapters (windows.h in win32/, unistd.h in posix/);
#   - layering (build/sources.sh): core includes core and config.h; app includes core,
#     app and platform interfaces; adapters include core, platform interfaces and their own
#     OS folder. Nothing includes app except app; nothing outside an OS folder includes it.
# Usage: build/checks/includes.sh [files...]   (default: every file under src/)
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../.."
STD='^(assert|ctype|errno|limits|stdarg|stddef|stdint|stdio|stdlib|string|time|wchar)\.h$' # C99, any layer
OS_WIN32='^(windows|wincred|wincrypt|winhttp)\.h$'
OS_POSIX='^(unistd|fcntl|termios|signal|poll|pwd|netdb|sys/(stat|types|wait|ioctl|utsname|time|socket)|mbedtls/(ssl|x509_crt|net_sockets|pk|md)|psa/crypto)\.h$' # + vendored Mbed TLS (ADR 0001)
os_headers() { # the OS headers a file may include: only its own platform's
  case "$1" in src/platform/win32/*) echo "$OS_WIN32" ;; src/platform/posix/*) echo "$OS_POSIX" ;; *) echo '^$' ;; esac
}
layer_of() { # header name -> layer of the file that defines it, or "" if not ours
  local f
  for f in src/*.h src/core/*.h src/app/*.h src/platform/*.h src/platform/*/*.h; do
    [[ "$(basename "$f")" == "$1" ]] || continue
    case "$f" in
      src/core/*) echo core ;; src/app/*) echo app ;; src/platform/*/*) echo "os:${f#src/platform/}" ;;
      src/platform/*) echo platform ;; *) echo config ;;
    esac
    return
  done
}
allowed() { # $1 = including file, $2 = layer of the included header
  case "$1" in
    src/core/*) [[ "$2" == core || "$2" == config ]] ;;
    src/app/*) [[ "$2" != os:* ]] ;;
    src/platform/*/*) [[ "$2" != app && ( "$2" != os:* || "$(dirname "${2#os:}")" == "$(dirname "${1#src/platform/}")" ) ]] ;;
    src/platform/*) [[ "$2" == core || "$2" == config || "$2" == platform ]] ;;
    *) true ;;
  esac
}
[ $# -gt 0 ] || set -- $(find src -name '*.[ch]' | sort)
fail=0
for f in "$@"; do
  [ -f "$f" ] || continue
  while IFS= read -r hdr; do
    if echo "$hdr" | grep -Eq "$STD" || echo "$hdr" | grep -Eq "$(os_headers "$f")"; then
      :
    elif [[ "$hdr" == */* ]]; then
      echo "check-includes: $f: no paths in includes ($hdr); folders go in build/sources.sh" >&2
      fail=1
    elif layer="$(layer_of "$hdr")" && [ -n "$layer" ]; then
      allowed "$f" "$layer" || { echo "check-includes: $f no puede incluir $hdr (capa $layer)" >&2; fail=1; }
    else
      echo "check-includes: $f incluye $hdr, fuera de la allowlist" >&2
      fail=1
    fi
  done < <(sed -n 's/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"]\([^>"]*\)[>"].*/\1/p' "$f")
done
exit "$fail"
