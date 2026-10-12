#!/usr/bin/env bash
# Fails on C APIs never accepted in TinyShell, with no exception markers: gets, sprintf (use
# snprintf), strtok, system(, popen(, wsprintf, ANSI (A) variants, lstrcpyA/lstrcatA, alloca.
# strcpy/strcat/lstrcpyW/lstrcatW are not listed: the tree uses them after a checked length
# guard (see credential_win32.c:target, provider.c:copy).
set -euo pipefail
fail=0
# Only code counts: strings, chars and comments are blanked in one pass (keeping line
# numbers), so prose like "the system (...)" in a prompt or a comment is not a call.
strip() {
  perl -0777 -pe 's{("(?:\\.|[^"\\])*"|'"'"'(?:\\.|[^'"'"'\\])*'"'"'|/\*.*?\*/|//[^\n]*)}{ (my $t = $1) =~ tr/\n//cd; $t }gse' "$1"
}
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
