# Operations runbook

## First launch

1. Build the x86 executable on a trusted development machine.
2. Scan the executable and copy it to a folder such as `C:\Tools\TinyShell`.
3. Open CMD as the normal user, not administrator.
4. Run `tinyshell.exe`; provide the OpenCode Go API key interactively.
5. Begin with a read-only task: `Decime versión de Windows, RAM disponible y espacio libre en C:. No cambies nada.`
6. Inspect every proposed command, including pipes, redirection, quoted arguments and nested commands.
7. Use `/salir` to finish.

## Troubleshooting sequence

### API call fails

1. Check date/time in Windows.
2. Check DNS and connectivity: `nslookup opencode.ai` and `ping opencode.ai` (ping may be blocked; that alone is not decisive).
3. Check HTTPS access in a supported browser if available.
4. Confirm API key, account access, endpoint and model name.
5. Check Windows 7 TLS 1.2 support, updates and trusted root certificates. Do not disable certificate validation as a workaround.

### Executable fails to start

- Confirm it is PE32 / x86, not x64.
- Check the compiler/runtime choice. MSVC `/MT` statically links the MSVC runtime, but system DLL requirements remain.
- Capture the exact error and compiler command.

### Command output is truncated or odd

- Use a smaller, targeted command.
- Avoid large recursive `dir /s`, full event logs, or unbounded listings on the low-memory PC.
- Remember CMD code pages may affect accented characters even with UTF-8 console mode.

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
