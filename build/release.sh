#!/usr/bin/env bash
set -euo pipefail

# Builds, signs and publishes a GitHub release from a clean main.
#   build/release.sh 0.4.1            publish
#   build/release.sh 0.4.1 --dry-run  build and sign into dist/ only
# Needs: mingw-w64, Docker (for the NSIS installer), openssl, gh (authenticated), the private
# key from gen-signing-key.sh.
# Before running: set TINYSHELL_VERSION in src/config.h and add a "## X.Y.Z" CHANGELOG entry.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
VERSION="${1:?usage: build/release.sh X.Y.Z [--dry-run]}"
DRY="${2:-}"
KEY="${TINYSHELL_SIGNING_KEY:-$HOME/.config/tinyshell/release-signing-key.pem}"
die() { echo "release: $*" >&2; exit 1; }

[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "invalid version: $VERSION"
grep -q "#define TINYSHELL_VERSION \"$VERSION\"" src/config.h || die "src/config.h does not say TINYSHELL_VERSION \"$VERSION\""
grep -q "^## $VERSION" CHANGELOG.md || die "CHANGELOG.md has no '## $VERSION' entry"
[[ -f "$KEY" ]] || die "private key $KEY not found (build/gen-signing-key.sh)"
grep -q "RELEASE_PUBLIC_KEY_LEN 0" src/release_key.h && die "src/release_key.h is still the placeholder"
diff -q <(openssl pkey -in "$KEY" -pubout) keys/release-public.pem >/dev/null ||
  die "the private key does not match keys/release-public.pem"
if [[ "$DRY" != --dry-run ]]; then
  [[ "$(git branch --show-current)" == main ]] || die "release from main"
  [[ -z "$(git status --porcelain)" ]] || die "there are uncommitted changes"
  git rev-parse -q --verify "refs/tags/v$VERSION" >/dev/null && die "tag v$VERSION already exists"
fi

./build/test-core.sh
./build/build-linux-mingw.sh
DIST="dist/v$VERSION"
rm -rf "$DIST" && mkdir -p "$DIST"
cp tinyshell.exe scripts/install.bat "$DIST/"
./build/build-installer.sh tinyshell.exe "$DIST/TinyShell-Setup.exe"
printf '%s\r\n' "$VERSION" >"$DIST/version.txt"
openssl dgst -sha256 -sign "$KEY" -out "$DIST/tinyshell.exe.sig" "$DIST/tinyshell.exe"
openssl dgst -sha256 -verify keys/release-public.pem -signature "$DIST/tinyshell.exe.sig" "$DIST/tinyshell.exe"
(cd "$DIST" && sha256sum tinyshell.exe TinyShell-Setup.exe tinyshell.exe.sig install.bat version.txt >SHA256SUMS)
cat "$DIST/SHA256SUMS"
if [[ "$DRY" == --dry-run ]]; then
  echo "Dry run: artifacts in $DIST (nothing was published)."
  exit 0
fi

NOTES="$(awk -v v="## $VERSION" 'index($0, v) == 1 {on = 1; next} /^## / {on = 0} on' CHANGELOG.md)"
git tag -a "v$VERSION" -m "TinyShell $VERSION"
git push origin main "v$VERSION"
gh release create "v$VERSION" "$DIST"/* --title "TinyShell $VERSION" \
  --notes "$NOTES"$'\n\n'"SHA-256 of TinyShell-Setup.exe: \`$(grep ' TinyShell-Setup.exe$' "$DIST/SHA256SUMS" | cut -d' ' -f1)\`"$'\n'"SHA-256 of tinyshell.exe: \`$(grep ' tinyshell.exe$' "$DIST/SHA256SUMS" | cut -d' ' -f1)\`"
echo "Published v$VERSION."
