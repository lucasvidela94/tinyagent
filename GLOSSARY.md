# Glosario

Términos del dominio de TinyShell. El código y los docs usan estos nombres.

- **Sesión** (`Session`): una conversación con el modelo: historial, proveedor, API key,
  contadores e ID de sesión.
- **Proveedor** (`Provider`): un endpoint compatible con OpenAI chat completions (nombre,
  URL, modelo y header de sesión). OpenCode Go es el preset por defecto.
- **Turno**: un ida y vuelta con el modelo. Un mensaje del usuario puede llevar hasta
  `MAX_TURNS` turnos.
- **Propuesta**: un comando que el modelo pide ejecutar con la herramienta `shell`, con su
  motivo. El modelo solo propone.
- **Aprobación**: la confirmación local escrita (`s` + Enter, y `EJECUTAR` si hay riesgo) sin
  la cual no se ejecuta nada.
- **Riesgo**: el motivo local (`risk_reason`) por el que un comando pide la confirmación
  fuerte. Lo decide el código, no el prompt.
- **Dialecto de shell**: lo que depende del shell de la plataforma: reglas de riesgo y guía
  del prompt (`cmd.exe` en Windows, `/bin/sh` en Linux).
- **Núcleo**: los módulos portables de `src/`, testeados en el host.
- **Seam de plataforma**: los headers de `src/` que cada plataforma implementa
  (`console`, `http`, `shell`, `credential`, `settings`, `cancel`, `sys`, `verify`).
- **Adaptador de plataforma**: la implementación de esos headers para una plataforma
  (`src/win32/`, `src/posix/`).
- **Release**: binario firmado con la clave RSA de releases, más `version.txt`, la firma y
  `SHA256SUMS`. `--update` solo instala lo que verifica y pasa el self-test.
- **Reporte**: lo que manda `/reporte`: el resumen público redactado (issue en GitHub) y el
  informe completo privado (D1).
- **Evento**: telemetría de la beta: solo metadatos, nunca lo que el usuario escribe.
- **Relay**: el Worker de Cloudflare que recibe reportes y eventos.
