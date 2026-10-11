# Target PC profile and implications

## Observed baseline

- Model: Noblex NT1013E.
- OS: Windows 7 Professional SP1, build 7601, x86 / 32-bit.
- CPU: Intel Atom N2600 @ 1.60 GHz; 2 cores and 4 logical processors reported.
- RAM: 2 GB (user-reported).

## Implications

- Build a **32-bit Windows PE**; do not ship an x64-only executable.
- Keep local code native C, avoid Electron, browser runtimes, local databases, container engines, and local LLMs.
- Use remote inference so CPU/RAM are not spent on model weights.
- Avoid persistent polling or background services; start only when the user asks.
- Limit output sizes and avoid reading whole disks or recursively enumerating the entire drive by default.
- Run as a standard user initially. Elevation increases the blast radius of an incorrect command.
- Network/API success depends on supported TLS protocols, root certificates, clock correctness, DNS, proxy/firewall, and API credentials.

## Initial non-destructive checks

Run `scripts/collect-diagnostics.bat` from a normal CMD window. It writes a timestamped report folder beside the script. Review the report before sharing: it can contain the hostname, usernames, installed software/processes, network configuration, IP addresses and device details.

## Unknowns to verify on the actual PC

- Free RAM and disk space at runtime.
- Whether Windows 7 has TLS 1.2 and current root certificates configured for WinHTTP.
- Whether the system clock is correct.
- Whether the network permits `opencode.ai:443`.
- Whether the installed Windows 7 image can run a binary produced by the selected compiler/toolchain.
