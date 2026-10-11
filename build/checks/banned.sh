#!/usr/bin/env bash
# Falla ante APIs C nunca aceptables en TinyShell. Sin excepciones con marcador:
# gets, sprintf (usar snprintf), strtok, system(, popen(, wsprintf, variantes ANSI (A),
# lstrcpyA/lstrcatA, alloca sin tope.
# strcpy/strcat/lstrcpyW/lstrcatW NO estan en esta lista: el arbol los usa con
# guarda de longitud previa verificada (ver credential_win32.c:target,
# provider.c:copy). Endurecerlos con marcador queda como paso futuro.
set -euo pipefail
fail=0
# Quita strings "..." para no marcar texto (p.ej. el prompt que dice "the system (...").
strip() { sed -e 's/"\(\\.\|[^"\\]\)*"//g' -e "s/'\(\\\\.\|[^'\\\\]\)*'//g" "$1"; }
HARD='\<(gets|sprintf|strtok|system|popen|wsprintf[A-Z]*|alloca)\s*\(|lstrcpyA|lstrcatA|\<(Create|Get|Set|Reg|Message|Shell|Copy|Move|Delete|Find)[A-Za-z]*A\s*\('
for f in "$@"; do
  [ -f "$f" ] || continue
  case "$f" in */release_key.h) continue ;; esac # blob generado, no auditable a mano
  if strip "$f" | grep -nE "$HARD" >/tmp/banned_$$ 2>/dev/null; then
    echo "banned-apis: $f" >&2
    cat /tmp/banned_$$ >&2
    fail=1
  fi
done
rm -f /tmp/banned_$$
exit "$fail"
