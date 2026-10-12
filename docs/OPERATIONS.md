# Operations runbook

## First launch

1. Run `TinyShell-Setup.exe` as the normal user (downloaded, or copied from a USB stick; the README also has one-line commands). Unsigned for now: Windows may say the publisher is unknown ("More info", "Run anyway" on Windows 10/11).
2. Compare the installer's SHA-256 with the release page (`certutil -hashfile TinyShell-Setup.exe SHA256`).
3. Keep "Set it up now" checked at the end, or later open TinyShell from the Start menu (or `tinyshell` in a **new** CMD window) and run `tinyshell login`.
4. Provide the API key when asked.
5. Begin with a read-only task: `Tell me the Windows version, the free RAM and the free space on C:. Do not change anything.`
6. Inspect every proposed command, including pipes, redirection, quoted arguments and nested commands.
7. Use `/quit` to finish.

## Updating

- `tinyshell --update` (or `/update` inside a session). Close and reopen TinyShell afterwards.
- "INVALID SIGNATURE" / "FIRMA INVÁLIDA": do not work around it; the release was not signed with the project key.
- "failed the start-up test" / "no pasó la prueba de arranque": the new build does not start on this machine; nothing was changed. Report it.
- `tinyshell --rollback` restores `tinyshell.old.exe`; running it again rolls forward.
- If the updater itself is broken, run the installer again (a USB copy works offline). Uninstall from Programs and Features.

## Troubleshooting sequence

### API call fails

1. Check date/time in Windows.
2. Check DNS and connectivity: `nslookup opencode.ai` and `ping opencode.ai` (ping may be blocked; that alone is not decisive).
3. Check HTTPS access in a supported browser if available.
4. Confirm API key, account access, endpoint and model name.
5. Check Windows 7 TLS 1.2 support, updates and trusted root certificates. Do not disable certificate validation as a workaround.
   - TinyShell prints the WinHTTP error code with a hint: 12007 DNS, 12029 cannot connect, 12002 timeout, 12030 connection reset (often TLS), 12175/12157 TLS failure, 12045 untrusted root, 12037 bad date.
   - TinyShell requests TLS 1.2 itself; this needs update **KB3140245**. If it is missing, a warning is printed at the first request. No `DefaultSecureProtocols` registry change is needed for TinyShell.

### Executable fails to start

- Confirm it is PE32 / x86, not x64.
- Check the compiler/runtime choice. MSVC `/MT` statically links the MSVC runtime, but system DLL requirements remain.
- A MinGW build must import only `KERNEL32.dll`, `msvcrt.dll` and `WINHTTP.dll`. If it imports `api-ms-win-crt-*.dll` (UCRT), rebuild with `build/build-linux-mingw.sh` or install KB2999226.
- Capture the exact error and compiler command.

### Command output is truncated or odd

- Use a smaller, targeted command.
- Avoid large recursive `dir /s`, full event logs, or unbounded listings on the low-memory PC.
- Output is capped at 24 KB (`[salida truncada: N bytes omitidos]`) and commands are killed after 180 s (`[tiempo agotado]`).
- Command output is converted from the OEM code page (CP850 on Spanish Windows) to UTF-8 unless it is already valid UTF-8.
- Commands run with stdin = NUL: anything that waits for input (`pause`, `date`, `chkdsk` prompts) gets EOF instead of hanging.

## Safe diagnostic examples

- `ver`
- `systeminfo`
- `wmic os get Caption,Version,OSArchitecture,LastBootUpTime`
- `wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed`
- `wmic OS get FreePhysicalMemory,TotalVisibleMemorySize`
- `wmic logicaldisk get DeviceID,FileSystem,FreeSpace,Size`
- `tasklist`
- `ipconfig /all` (contains network identifiers; redact before sharing)

Avoid repair switches such as `chkdsk /f`, registry changes, driver installs, service changes, disk tools, deletes, and resets until the diagnosis and consequences are understood.
