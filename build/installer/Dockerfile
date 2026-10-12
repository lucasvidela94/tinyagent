# NSIS for building the Windows installer on Linux (makensis is native; the installers it
# makes run on Windows 95 and later). Used by build/build-installer.sh.
FROM debian:bookworm-slim
RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends nsis && rm -rf /var/lib/apt/lists/*
WORKDIR /src
