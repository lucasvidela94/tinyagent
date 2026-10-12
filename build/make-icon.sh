#!/usr/bin/env bash
set -euo pipefail

# Regenerates assets/tinyshell.ico from assets/tinyshell.svg (the site's favicon, so the
# program, the installer and the site share one icon). PNG-compressed entries keep it near
# 4 KB inside the exe; Windows Vista and later read them. Needs rsvg-convert and Python PIL.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
rsvg-convert -w 256 -h 256 "$ROOT/assets/tinyshell.svg" -o "$TMP/256.png"
python3 -I -c 'import sys; from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2], format="ICO", sizes=[(16,16),(24,24),(32,32),(48,48),(256,256)])' \
  "$TMP/256.png" "$ROOT/assets/tinyshell.ico"
echo "Wrote $ROOT/assets/tinyshell.ico ($(wc -c <"$ROOT/assets/tinyshell.ico") bytes)"
