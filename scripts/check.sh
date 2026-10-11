#!/usr/bin/env bash
# Every local check in one place: the husky pre-commit hook and CI both run this.
#   scripts/check.sh          static checks + host tests
#   scripts/check.sh --build  also the -Werror Win32 cross-build and the exe budget
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
bash scripts/check-banned.sh src/*.c src/*.h
bash scripts/check-size.sh src/*.c src/*.h tests/*.c
bash scripts/check-includes.sh src/*.c src/*.h
bash scripts/check-docs.sh
./build/test-core.sh
if [[ "${1:-}" == --build ]]; then
  ./build/build-linux-mingw.sh
  bash scripts/check-exe-size.sh tinyshell.exe
fi
echo "check: OK"
