#!/usr/bin/env bash
# Presupuesto del .exe: un solo binario, tamano verificable.
# Actual: ~110 KB (2026-10-11). Techo 128 KB; bajarlo requiere justificacion, subirlo no.
set -euo pipefail
MAX="${MAX_EXE:-131072}"
f="${1:-tinyshell.exe}"
[ -f "$f" ] || { echo "check-exe-size: no existe $f (compilar primero)" >&2; exit 1; }
n=$(wc -c <"$f")
if [ "$n" -gt "$MAX" ]; then
  echo "check-exe-size: $f pesa $n bytes (techo $MAX). Recortar o justificar." >&2
  exit 1
fi
echo "check-exe-size: $f $n bytes (techo $MAX) OK"
