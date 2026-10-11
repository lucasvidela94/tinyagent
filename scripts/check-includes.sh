#!/usr/bin/env bash
# Allowlist de headers: "sin dependencias" verificable, no promesa del README.
# Sistema: assert ctype errno limits stdarg stddef stdint stdio stdlib string wchar
#   windows.h wincred wincrypt winhttp | Proyecto: *.h (mismo repo).
set -euo pipefail
SYS='^(assert|ctype|errno|limits|stdarg|stddef|stdint|stdio|stdlib|string|wchar|windows|wincred|wincrypt|winhttp)\.h$'
fail=0
for f in "$@"; do
  [ -f "$f" ] || continue
  while IFS= read -r inc; do
    hdr="${inc#*include}"
    hdr="$(echo "$hdr" | tr -d ' "<>')"
    case "$hdr" in
      *.h) ;;
      *) continue ;;
    esac
    if [[ "$hdr" == */* ]]; then
      echo "check-includes: $f incluye ruta con subdirectorio: $hdr" >&2
      fail=1
      continue
    fi
    if [[ "$hdr" == *"_win32.h" || "$hdr" == "win32.h" ]]; then continue; fi
    if echo "$hdr" | grep -Eq "$SYS"; then continue; fi
    # header propio del repo
    if [ -f "src/$hdr" ]; then continue; fi
    echo "check-includes: $f incluye header fuera de la allowlist: $hdr" >&2
    fail=1
  done < <(grep -E '#include' "$f" || true)
done
exit "$fail"
