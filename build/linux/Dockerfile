# Build environment for the static Linux binary (docs/adr/0001-linux-port-platform-seam.md).
# Everything is pinned, so the same image can be rebuilt later:
#   - musl-cross-make at a fixed commit: GCC 13.3.0 and musl 1.2.6 for i586-linux-musl,
#     with i586 as the default CPU, so libc itself needs no SSE2 (Pentium/K6 and newer);
#   - Mbed TLS 4.1.1 (LTS until 2029-03), checked against the published SHA-256 and built
#     with the same compiler.
# Sources are fetched one at a time (parallel requests to GNU mirrors fail at random).
# Used by build/build-linux-musl.sh; nothing from here ships except the binary it builds.
FROM debian:bookworm-slim AS toolchain
RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
      build-essential ca-certificates curl wget git xz-utils bzip2 rsync cmake python3 \
    && rm -rf /var/lib/apt/lists/*
ARG MCM_COMMIT=227df8b99103f9c59f6570babf892978e293082f
RUN git clone -q https://github.com/richfelker/musl-cross-make /mcm && cd /mcm && git checkout -q "$MCM_COMMIT"
RUN cd /mcm && printf '%s\n' \
      'TARGET = i586-linux-musl' \
      'GCC_VER = 13.3.0' \
      'MUSL_VER = 1.2.6' \
      'COMMON_CONFIG += CFLAGS="-g0 -O2" CXXFLAGS="-g0 -O2"' \
      'GCC_CONFIG += --with-arch=i586 --with-tune=generic --enable-languages=c --disable-multilib' \
      'GNU_SITE = https://ftp.gnu.org/gnu' \
      > config.mak \
    && { make extract_all >/tmp/fetch.log 2>&1 || make extract_all >/tmp/fetch.log 2>&1 \
         || { grep -v ' \.\.\.\.\.' /tmp/fetch.log | tail -20; exit 1; }; } \
    && { make -j"$(nproc)" >/tmp/mcm.log 2>&1 && make install OUTPUT=/opt/cross >>/tmp/mcm.log 2>&1 \
         || { grep -nE 'error|Error [0-9]' /tmp/mcm.log | head -20; exit 1; }; } \
    && rm -rf /mcm
ENV PATH=/opt/cross/bin:$PATH

ARG MBEDTLS_VER=4.1.1
ARG MBEDTLS_SHA256=3359a349e23db3d5536fcee032ae7b2ecbfc08972fab643089b5cbf2a375c98c
RUN curl -fsSL -o /tmp/mbedtls.tar.bz2 \
      "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-$MBEDTLS_VER/mbedtls-$MBEDTLS_VER.tar.bz2" \
    && echo "$MBEDTLS_SHA256  /tmp/mbedtls.tar.bz2" | sha256sum -c - \
    && mkdir /mbedtls && tar -xjf /tmp/mbedtls.tar.bz2 -C /mbedtls --strip-components=1 \
    && cmake -S /mbedtls -B /mbedtls/build -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=i586 \
         -DCMAKE_C_COMPILER=i586-linux-musl-gcc -DCMAKE_C_FLAGS="-Os -march=i586" \
         -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF \
         -DUSE_SHARED_MBEDTLS_LIBRARY=OFF -DCMAKE_INSTALL_PREFIX=/opt/mbedtls >/tmp/mbedtls.log \
    && { cmake --build /mbedtls/build -j"$(nproc)" >>/tmp/mbedtls.log && cmake --install /mbedtls/build >>/tmp/mbedtls.log \
         || { tail -40 /tmp/mbedtls.log; exit 1; }; } \
    && rm -rf /mbedtls /tmp/mbedtls.tar.bz2
WORKDIR /src
