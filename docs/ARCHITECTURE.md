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
  └── ejecutor cmd.exe (_popen)
           │ stdout/stderr + exit code
           └──────────────────────────────► historial / siguiente turno
```

## Source map

- `agent.c`
  - Configuración de endpoint y modelo (`API_HOST`, `API_PATH`, `MODEL`).
  - Helpers de cadenas/JSON: `jesc`, `parse_json`, `key`, `tokstr`, `slice`.
  - HTTP: `http_post`, `read_all`.
  - Conversación: `history`, `build_request`, bucle principal de `main`.
  - Aprobación y ejecución local: `dangerous`, `run_shell`, `add_tool_result`.
- `build/build-linux-mingw.sh`: cross-compile Linux → Windows x86.
- `build/build-windows-msvc.bat`: build desde Developer Command Prompt de MSVC.
- `scripts/collect-diagnostics.bat`: diagnóstico local separado; no necesita API.
- `docs/`: decisiones, operación, seguridad, pruebas y hoja de ruta.

## Data flow

1. La persona escribe un objetivo.
2. El programa añade el mensaje al historial y lo envía por HTTPS.
3. El modelo puede devolver una respuesta normal o una llamada a `shell`.
4. Si devuelve `shell`, el programa muestra comando y motivo.
5. Si se rechaza, devuelve al modelo que el usuario lo rechazó; si se aprueba, ejecuta mediante `cmd.exe`.
6. El resultado vuelve como mensaje de herramienta y el modelo puede continuar, dentro de un límite de turnos.

## Constraints / design choices

- Arquitectura objetivo: x86 de 32 bits; compilar con `i686-w64-mingw32-gcc` o toolchain x86 de MSVC.
- HTTP: WinHTTP del sistema, TLS/certificados dependen de la instalación de Windows 7.
- JSON: parser embebido pequeño, sin dependencia externa; debe considerarse código sensible y probarse contra respuestas inesperadas.
- Ejecución: `cmd.exe` vía `_popen`; no es un sandbox y el quoting de comandos tiene límites.
- Memoria: límites de respuesta/historial deben vigilarse. El código actual acumula historial durante la sesión.

## Recommended future module boundaries

Mantener el primer build simple; si crece, separar en módulos C:

- `src/config.h`: endpoint/model/constantes.
- `src/json.c/.h`: parser y escaping, con pruebas unitarias.
- `src/http_winhttp.c/.h`: transporte HTTP y errores.
- `src/agent.c/.h`: protocolo de mensajes y límite de iteraciones.
- `src/approval.c/.h`: políticas de aprobación y reglas.
- `src/shell_win32.c/.h`: ejecución y captura de salida.
- `src/main.c`: CLI.

No separar en microservicios: la máquina destino debe seguir siendo simple.
