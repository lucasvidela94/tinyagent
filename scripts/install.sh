#!/bin/sh
# TinyShell installer for Linux (x86, 32- or 64-bit). POSIX sh, so it also runs on dash and
# busybox. Downloads the latest release's static binary, checks it against SHA256SUMS,
# installs it as ~/.local/bin/tinyshell (or $TINYSHELL_DIR) and checks that it starts.
#   sh install.sh            download and install
#   sh install.sh ./file     install a binary you already have (e.g. copied from a USB stick)
# Later updates: tinyshell --update (verifies the release signature itself).
set -eu

BASE="https://github.com/lucasvidela94/tinyagent/releases/latest/download"
ASSET="tinyshell-linux-i386"
DIR="${TINYSHELL_DIR:-$HOME/.local/bin}"
TMP="${TMPDIR:-/tmp}/tinyshell-install.$$"

case "${LC_ALL:-}${LC_MESSAGES:-}${LANG:-}" in es*) ES=1 ;; *) ES= ;; esac
say() { if [ -n "$ES" ]; then printf '%s\n' "$2"; else printf '%s\n' "$1"; fi; }
fail() { say "$1" "$2" >&2; rm -rf "$TMP"; exit 1; }

case "$(uname -m)" in
  i?86 | x86_64 | amd64) ;;
  *) fail "This build is for x86 PCs (i586 and newer); this one is $(uname -m)." \
          "Esta versión es para PCs x86 (i586 o posterior); este equipo es $(uname -m)." ;;
esac

fetch() { # url file
  if command -v curl >/dev/null 2>&1; then curl -fsSL -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then wget -q -O "$2" "$1"
  else return 1; fi
}

sha256() { # file -> hex digest
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
  elif command -v openssl >/dev/null 2>&1; then openssl dgst -sha256 "$1" | sed 's/.*= //'
  else echo ""; fi
}

mkdir -p "$TMP"
if [ $# -gt 0 ]; then
  [ -f "$1" ] || fail "$1 does not exist." "$1 no existe."
  cp "$1" "$TMP/$ASSET"
  say "Installing the local copy: $1" "Instalando la copia local: $1"
else
  say "Downloading TinyShell..." "Descargando TinyShell..."
  if ! fetch "$BASE/$ASSET" "$TMP/$ASSET" || ! fetch "$BASE/SHA256SUMS" "$TMP/SHA256SUMS"; then
    fail "Could not download it. Very old distros often cannot reach GitHub (old TLS). Download
$ASSET on another PC from https://github.com/lucasvidela94/tinyagent/releases/latest
and run: sh install.sh ./$ASSET" \
         "No se pudo descargar. Las distros muy viejas suelen no poder conectarse a GitHub (TLS viejo).
Bajá $ASSET en otra PC desde https://github.com/lucasvidela94/tinyagent/releases/latest
y ejecutá: sh install.sh ./$ASSET"
  fi
  want="$(grep " $ASSET\$" "$TMP/SHA256SUMS" | cut -d' ' -f1)"
  got="$(sha256 "$TMP/$ASSET")"
  if [ -z "$got" ]; then
    say "No sha256sum or openssl here: the checksum was not verified." \
        "No hay sha256sum ni openssl: no se verificó el checksum."
  elif [ "$want" != "$got" ]; then
    fail "The download does not match SHA256SUMS. Nothing was installed." \
         "La descarga no coincide con SHA256SUMS. No se instaló nada."
  fi
fi

chmod 755 "$TMP/$ASSET"
version="$("$TMP/$ASSET" --version 2>/dev/null)" ||
  fail "The binary does not start on this system. Nothing was installed." \
       "El binario no arranca en este sistema. No se instaló nada."
mkdir -p "$DIR"
mv -f "$TMP/$ASSET" "$DIR/tinyshell"
rm -rf "$TMP"
say "Installed $version in $DIR/tinyshell" "Instalado $version en $DIR/tinyshell"

case ":$PATH:" in
  *":$DIR:"*) say "Next: tinyshell login" "Siguiente: tinyshell login" ;;
  *) say "$DIR is not in your PATH. Add this line to ~/.profile and open a new terminal:
  export PATH=\"$DIR:\$PATH\"
Meanwhile: $DIR/tinyshell login" \
         "$DIR no está en tu PATH. Agregá esta línea a ~/.profile y abrí una terminal nueva:
  export PATH=\"$DIR:\$PATH\"
Mientras tanto: $DIR/tinyshell login" ;;
esac
