# Codebase architecture

## Architectural style

**Single-process, synchronous CLI / thin remote-agent harness.** C nativo + WinHTTP + `cmd.exe`. No servidor local, daemon, DB, framework ni runtime administrado. El proveedor remoto genera propuestas; el ejecutable es el mediador local que pide permiso y ejecuta el comando.

```text
Persona
  │ texto en español / confirmación local
  ▼
CLI (main)
  ├── historial de mensajes JSON
  ├── cliente HTTP(S) WinHTTP ───────────► API compatible con OpenAI (config.ini)
  │                                         por defecto OpenCode Go / deepseek-v4.1-flash
  │◄──────────────── respuesta JSON / tool call
  ├── parser JSON mínimo
  ├── control local de aprobación
  └── ejecutor cmd.exe (CreateProcessW + pipe, stdin = NUL, timeout)
           │ stdout/stderr + exit code
           └──────────────────────────────► historial / siguiente turno
```

## Source map

`src/` está ordenado por capas (rutas en `build/sources.sh`; decisión en
`docs/adr/0001-linux-port-platform-seam.md`):

```
src/config.h, release_key.h   constantes de build
src/core/                     portable, testeado en el host (build/test-core.sh)
src/app/                      el programa: sesión, TUI, login, reportes, main
src/platform/                 el seam de plataforma: un .h por servicio del sistema
src/platform/win32/           adaptadores Windows
src/platform/posix/           adaptadores Linux (en curso: sys, consola, shell y Ctrl+C listos)
```

Regla de dependencias, verificada en cada commit por `build/checks/includes.sh`:
`core` solo incluye `core` y `config.h`; `platform/*.h` incluye `core`; `app` incluye
`core`, `app` y `platform`, nunca un adaptador; un adaptador incluye `core`, `platform` y
su propia carpeta, nunca `app` ni otra plataforma. Los `#include` no llevan rutas.

Un módulo por archivo, cada uno con una interfaz chica en su `.h`. Regla de mantenimiento: ningún archivo pasa de ~300 líneas (objetivo ~200); si crece, se divide por una interfaz real, no por tamaño.

| Módulo | Interfaz | Responsabilidad | Plataforma |
|---|---|---|---|
| `src/config.h` | constantes | endpoint, modelo, límites (`MAX_HTTP`, `MAX_HISTORY`, `MAX_OUTPUT`, `MAX_COMMAND`, `CMD_TIMEOUT_MS`…) | — |
| `src/core/util.c` | `Buf`, `utf8_valid`, `jesc` | buffers, validación UTF-8, escape JSON (bytes inválidos → `\ufffd`) | portable |
| `src/core/json.c` | `json_parse/key/index/str` | tokenizer acotado (profundidad, tokens), `\uXXXX` → UTF-8 | portable |
| `src/core/conversation.c` | `History`, `build_request`, `ToolCall` | historial con recorte por intercambios completos, body de la request, extracción de tool calls | portable |
| `src/core/provider.c` | `provider_defaults`, `provider_set` | proveedor (nombre, URL, modelo, header de sesión), valores por defecto y validación de `config.ini` | portable |
| `src/core/report.c` | `errlog_*`, `redact`, `report_json`, `event_json` | errores recientes, limpieza de keys y datos personales, cuerpos para el relay | portable |
| `src/core/stream.c` | `stream_feed`, `stream_response` | streaming SSE: texto en vivo, une los pedazos de tool calls, rearma la respuesta normal | portable |
| `src/core/version.c` | `version_compare`, `version_from_output` | versiones X.Y.Z y salida de `--version` para el updater | portable |
| `src/core/charset.c` | `charset_encode`, `charset_decode`, `charset_from_locale` | texto para terminales que no son UTF-8 (`LANG=C` o Latin-1 en distros viejas) | portable |
| `src/core/command.c` | `command_result` | el texto con que vuelve al modelo el resultado de un comando, igual en todas las plataformas | portable |
| `src/core/dialect.h`, `dialect_cmd.c`, `dialect_sh.c` | `DIALECT_CMD`, `DIALECT_SH` | lo que depende del shell, como datos: reglas de riesgo y guía del prompt para `cmd.exe` y `/bin/sh` | portable |
| `src/core/instructions.c` | `instructions` | instrucciones agnósticas de tarea del prompt, con las partes del dialecto | portable |
| `src/core/policy.c` | `command_problem`, `risk_reason`, `is_yes` | rechazo previo, heurística de riesgo, respuesta de aprobación | portable |
| `src/platform/win32/console_win32.c` | `say`, `read_line`, `console_*` | E/S de consola UTF-16 (sin cambiar code page), líneas largas rechazadas; colores, ancho, cursor, borrar filas, vaciar entrada | Win32 |
| `src/platform/win32/settings_win32.c` | `settings_load`, `settings_save_model` | `%LOCALAPPDATA%\TinyShell\config.ini` (`GetPrivateProfileStringW`), plantilla comentada en el primer arranque | Win32 |
| `src/app/tui.c` | `tui_prompt`, `tui_user`, `tui_proposal`, `tui_output`… | TUI mínima en línea: bloque del usuario, propuesta, resumen de salida, editor entre reglas, footer; modo plano si no hay consola | portable |
| `src/platform/win32/console_input_win32.c` | `read_line`, `read_secret`, `read_key` | entrada: líneas, keys con `*`, teclas para menús | Win32 |
| `src/app/tui_select.c` | `tui_select` | menú con flechas, filtro tipeando y ventana de 10 filas | portable |
| `src/app/login.c` | `login_run`, `login_pick_model` | `tinyshell login`: proveedor, key (validada con `/models`), modelo de la lista en vivo | portable |
| `src/app/reporter.c` | `reporter_run` | `/reporte`: arma, guarda en `.txt`, muestra y envía el informe | portable |
| `src/app/telemetry.c` | `telemetry_init/event/error` | telemetría de la beta: aviso, `telemetria on/off/--ver`, registro local de lo enviado | portable |
| `src/platform/win32/cancel_win32.c` | `cancel_*` | Ctrl+C: cierra el pedido HTTP o mata el comando desde el hilo del handler, con handles protegidos; corta la lectura pendiente | Win32 |
| `src/platform/win32/credential_win32.c` | `credential_load/save/delete` | API key opcional en el Administrador de credenciales (DPAPI por usuario) | Win32 |
| `src/app/session.c` | `session_send` | historial, gate de aprobación (`decide`), bucle de turnos, conteo de tokens | portable |
| `src/platform/win32/sys_win32.c` | `sys_random`, `sys_ms`, `sys_data_path`, `sys_read_file`, `sys_rename`… | primitivas del sistema con rutas UTF-8: random, reloj, carpeta actual, carpeta de datos (`%LOCALAPPDATA%\TinyShell`), archivos, identidad, ruta propia y renombrar un binario en uso | Win32 |
| `src/platform/win32/http_win32.c` | `http_post_stream`, `http_get`, `http_post_json` | WinHTTP sobre cualquier URL http(s) (`WinHttpCrackUrl`), TLS 1.2 explícito, errores legibles, cuerpo acotado; GET de releases solo por HTTPS | Win32 |
| `src/platform/win32/shell_win32.c` | `run_command` | `cmd.exe /d /s /c` vía `CreateProcessW`, job + timeout, salida OEM → UTF-8 | Win32 |
| `src/platform/posix/sys_posix.c` | igual que `sys_win32` | `/dev/urandom`, `CLOCK_MONOTONIC`, `~/.config/tinyshell` (0700), archivos 0600, `/proc/self/exe` | POSIX |
| `src/platform/posix/console_posix.c`, `console_input_posix.c` | igual que `console_win32` | escapes ANSI, posición del cursor con `ESC[6n`, termios (modo canónico para líneas, carácter a carácter para claves y menús), charset del locale | POSIX |
| `src/platform/posix/shell_posix.c` | `run_command` | `/bin/sh -c` en su propio grupo de procesos; timeout y Ctrl+C matan el grupo | POSIX |
| `src/platform/posix/cancel_posix.c` | `cancel_*` | SIGINT sin `SA_RESTART` (corta la lectura pendiente), SIGPIPE ignorado | POSIX |
| `src/platform/win32/machine_win32.c` | `system_os`, `system_details`, `system_dialect` | equipo detectado al arrancar (versión de Windows, arquitectura, RAM, CPUs, carpeta actual) y el dialecto `cmd` | Win32 |
| `src/platform/win32/verify_win32.c` | `signature_valid`, `sha256_hex` | firma RSA-3072/SHA-256 de releases con la CryptoAPI; clave pública en `src/release_key.h` | Win32 |
| `src/app/update.c` | `update_run`, `update_rollback` | `--update`: version.txt → descarga → firma → self-test → swap con el hermano `.old` (`version_sibling`); `--rollback`. Usa `sys` para ruta propia, renombrar y el asset del release | portable |
| `src/app/main.c` | `main` | subcomandos (`login`, `logout`) y flags (`--version/--update/--rollback`), API key, bucle de chat y comandos `/` | Win32 |

Los módulos portables no incluyen `<windows.h>` y se prueban en el host con `build/test-core.sh` (`tests/`). Los adapters Win32 tienen un solo adapter cada uno: el seam es el `.h`, no hay capa de abstracción extra.

Otros archivos:

- `build/build-linux-mingw.sh`: cross-compile Linux → Windows x86, enlaza `msvcrt.dll` (no UCRT).
- `build/build-windows-msvc.bat`: build desde Developer Command Prompt de MSVC.
- `build/test-core.sh`: tests de los módulos portables (ASan/UBSan si están disponibles).
- `build/gen-signing-key.sh`: crea una vez el par de claves de releases (privada fuera del repo).
- `build/release.sh`: tests, build, firma, tag y `gh release create` (`--dry-run` disponible).
- `relay/`: Cloudflare Worker + D1 que recibe reportes (issue público con resumen + informe completo privado) y telemetría.
- `scripts/install.bat`: instalador publicado en cada release (curl/bitsadmin o copia local por USB).
- `scripts/collect-diagnostics.bat`: diagnóstico local separado; no necesita API.
- `docs/`: decisiones, operación, seguridad, pruebas y hoja de ruta.

## Data flow

1. La persona escribe un objetivo.
2. El programa añade el mensaje al historial y lo envía por HTTPS.
3. El modelo puede devolver una respuesta normal o una llamada a `shell`.
4. Si devuelve `shell`, el programa muestra comando y motivo.
5. Si se rechaza, devuelve al modelo que el usuario lo rechazó; si se aprueba, ejecuta mediante `cmd.exe`.
6. El resultado vuelve como mensaje de herramienta y el modelo puede continuar, dentro de un límite de 25 pasos por mensaje (`MAX_TURNS`).
7. Antes de cada request se recorta el historial a ~192 KB descartando intercambios completos (nunca separa un `tool_calls` de sus resultados).

## Constraints / design choices

- Arquitectura objetivo: x86 de 32 bits; compilar con `i686-w64-mingw32-gcc` o toolchain x86 de MSVC.
- HTTP: WinHTTP del sistema, TLS/certificados dependen de la instalación de Windows 7.
- JSON: parser embebido pequeño, sin dependencia externa; debe considerarse código sensible y probarse contra respuestas inesperadas.
- Ejecución: `cmd.exe /d /s /c` vía `CreateProcessW`; no es un sandbox y el quoting de comandos tiene límites.
- Memoria: respuesta ≤ 1 MB (`MAX_HTTP`), historial ~192 KB (`MAX_HISTORY`, blando: el último intercambio siempre se conserva), salida de comando ≤ 24 KB (`MAX_OUTPUT`), comando ≤ 2000 bytes (`MAX_COMMAND`), JSON ≤ 20 000 tokens (`MAX_TOKENS`) y 64 niveles (`MAX_DEPTH`), comandos con límite de 180 s (`CMD_TIMEOUT_MS`).
- Texto: todo es UTF-8 internamente; la consola usa la API UTF-16 y la salida de comandos se convierte desde el code page OEM.

## Future changes

- No separar en microservicios ni agregar runtimes: la máquina destino debe seguir siendo simple.
- Un segundo adapter (p. ej. una herramienta de solo lectura de archivos) recién justifica generalizar el despacho de herramientas en `main.c`; con una sola herramienta, no.
