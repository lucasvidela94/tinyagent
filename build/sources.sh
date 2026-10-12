# Source layout, sourced by the build, test, fuzz and check scripts (one place for paths).
# Layers (enforced by build/checks/includes.sh):
#   src/core              portable, host-tested; includes only core and config.h
#   src/app               the program; includes core, app and the platform interfaces
#   src/platform          the platform seam: one header per OS service
#   src/platform/<os>     adapters for one OS; include core and the platform interfaces
CORE_SRC=(src/core/*.c)
APP_SRC=(src/app/*.c)
WIN32_SRC=(src/platform/win32/*.c)
# POSIX adapters without TLS are tested natively on the host; the ones that need Mbed TLS
# are built with the Linux toolchain (build/build-linux-musl.sh).
POSIX_TLS_SRC=(src/platform/posix/conn_posix.c src/platform/posix/ca_bundle.c src/platform/posix/http_posix.c
  src/platform/posix/verify_posix.c)
POSIX_SRC=()
for f in src/platform/posix/*.c; do
  [[ " ${POSIX_TLS_SRC[*]} " == *" $f "* ]] || POSIX_SRC+=("$f")
done
INCLUDES=(-Isrc -Isrc/core -Isrc/app -Isrc/platform)
