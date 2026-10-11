# Codebase architecture

## Architectural style

**Single-process, synchronous CLI / thin remote-agent harness.** C nativo + WinHTTP + `cmd.exe`. No servidor local, daemon, DB, framework ni runtime administrado. El proveedor remoto genera propuestas; el ejecutable es el mediador local que pide permiso y ejecuta el comando.

```text
Persona
  │ texto en español / confirmación local
  ▼
CLI (main)
  ├── historial de mensajes JSON
  ├── cliente HTTPS WinHTTP ─────────────► OpenCode Go Chat Completions
  │                                         modelo deepseek-v4.1-flash
  │◄──────────────── respuesta JSON / tool call
  ├── parser JSON mínimo
  ├── control local de aprobación
  └── ejecutor cmd.exe (CreateProcessW + pipe, stdin = NUL, timeout)
           │ stdout/stderr + exit code
           └──────────────────────────────► historial / siguiente turno
```

## Source map

Un módulo por archivo, cada uno con una interfaz chica en su `.h`. Regla de mantenimiento: ningún archivo pasa de ~300 líneas (objetivo ~200); si crece, se divide por una interfaz real, no por tamaño.

| Módulo | Interfaz | Responsabilidad | Plataforma |
|---|---|---|---|
| `src/config.h` | constantes | endpoint, modelo, límites (`MAX_HTTP`, `MAX_HISTORY`, `MAX_OUTPUT`, `MAX_COMMAND`, `CMD_TIMEOUT_MS`…) | — |
| `src/util.c` | `Buf`, `utf8_valid`, `jesc` | buffers, validación UTF-8, escape JSON (bytes inválidos → `\ufffd`) | portable |
| `src/json.c` | `json_parse/key/index/str` | tokenizer acotado (profundidad, tokens), `\uXXXX` → UTF-8 | portable |
| `src/conversation.c` | `History`, `build_request`, `ToolCall` | historial con recorte por intercambios completos, body de la request, extracción de tool calls | portable |
| `src/policy.c` | `command_problem`, `risk_reason`, `is_yes` | rechazo previo, heurística de riesgo, respuesta de aprobación | portable |
| `src/console_win32.c` | `say`, `read_line` | E/S de consola UTF-16 (sin cambiar code page), líneas largas rechazadas | Win32 |
| `src/http_win32.c` | `http_post` | WinHTTP, TLS 1.2 explícito, errores legibles, cuerpo acotado | Win32 |
| `src/shell_win32.c` | `run_command` | `cmd.exe /d /s /c` vía `CreateProcessW`, job + timeout, salida OEM → UTF-8 | Win32 |
| `src/prompt_win32.c` | `system_prompt` | instrucciones agnósticas de tarea + equipo detectado al arrancar (versión de Windows, arquitectura, RAM, CPUs, carpeta actual) | Win32 |
| `src/main.c` | `main` | gate de aprobación (`decide`), bucle de turnos (`converse`) | Win32 |

Los módulos portables no incluyen `<windows.h>` y se prueban en el host con `build/test-core.sh` (`tests/`). Los adapters Win32 tienen un solo adapter cada uno: el seam es el `.h`, no hay capa de abstracción extra.

Otros archivos:

- `build/build-linux-mingw.sh`: cross-compile Linux → Windows x86, enlaza `msvcrt.dll` (no UCRT).
- `build/build-windows-msvc.bat`: build desde Developer Command Prompt de MSVC.
- `build/test-core.sh`: tests de los módulos portables (ASan/UBSan si están disponibles).
- `scripts/collect-diagnostics.bat`: diagnóstico local separado; no necesita API.
- `docs/`: decisiones, operación, seguridad, pruebas y hoja de ruta.

## Data flow

1. La persona escribe un objetivo.
2. El programa añade el mensaje al historial y lo envía por HTTPS.
3. El modelo puede devolver una respuesta normal o una llamada a `shell`.
4. Si devuelve `shell`, el programa muestra comando y motivo.
5. Si se rechaza, devuelve al modelo que el usuario lo rechazó; si se aprueba, ejecuta mediante `cmd.exe`.
6. El resultado vuelve como mensaje de herramienta y el modelo puede continuar, dentro de un límite de 8 turnos por mensaje.
7. Antes de cada request se recorta el historial a ~192 KB descartando intercambios completos (nunca separa un `tool_calls` de sus resultados).

## Constraints / design choices

- Arquitectura objetivo: x86 de 32 bits; compilar con `i686-w64-mingw32-gcc` o toolchain x86 de MSVC.
- HTTP: WinHTTP del sistema, TLS/certificados dependen de la instalación de Windows 7.
- JSON: parser embebido pequeño, sin dependencia externa; debe considerarse código sensible y probarse contra respuestas inesperadas.
- Ejecución: `cmd.exe /d /s /c` vía `CreateProcessW`; no es un sandbox y el quoting de comandos tiene límites.
- Memoria: respuesta ≤ 1 MB, historial ~192 KB (blando: el último intercambio siempre se conserva), salida de comando ≤ 24 KB, comando ≤ 2000 bytes, JSON ≤ 20 000 tokens y 64 niveles.
- Texto: todo es UTF-8 internamente; la consola usa la API UTF-16 y la salida de comandos se convierte desde el code page OEM.

## Future changes

- No separar en microservicios ni agregar runtimes: la máquina destino debe seguir siendo simple.
- Un segundo adapter (p. ej. una herramienta de solo lectura de archivos) recién justifica generalizar el despacho de herramientas en `main.c`; con una sola herramienta, no.
