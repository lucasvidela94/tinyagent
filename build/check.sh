#!/usr/bin/env bash
# Every local check in one place: the husky pre-commit hook and CI both run this.
#   build/check.sh          static checks + host tests
#   build/check.sh --build  also the -Werror Win32 cross-build and the exe budget
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
mapfile -t SRC < <(find src -name '*.[ch]' | sort)
bash build/checks/banned.sh "${SRC[@]}"
bash build/checks/size.sh "${SRC[@]}" tests/*.c tests/fuzz/*.c
bash build/checks/includes.sh "${SRC[@]}"
bash build/checks/docs.sh
bash build/checks/i18n.sh
./build/test-core.sh
if command -v uv >/dev/null 2>&1 || python3 -c 'import pyte' 2>/dev/null; then
  ./build/tui-test.sh
else
  echo "check: WARNING: no pyte/uv, the TUI was not tested in a pty (CI tests it)" >&2
fi
if [[ "${1:-}" == --build ]]; then
  ./build/build-linux-mingw.sh
  bash build/checks/exe-size.sh tinyshell.exe
fi
echo "check: OK"
