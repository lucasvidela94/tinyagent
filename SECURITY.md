# Política de seguridad

## Reportar una vulnerabilidad

**No abras un issue público** para vulnerabilidades (ejecución de comandos sin aprobación, bypass de firma de updates, robo de API keys, inyección vía respuesta del modelo).

Escribí a **lucasan.videla@gmail.com** con:

- Versión (`tinyshell --version`) y Windows afectado.
- Descripción del problema e impacto estimado.
- Pasos mínimos para reproducirlo (o PoC).
- Si podés, qué componente creés que falla (`src/...`).

Vas a recibir acuse de recibo y aviso cuando haya fix o release. Si querés, te credito en el `CHANGELOG.md`.

## Alcance

- El binario `tinyshell.exe`, `scripts/install.bat` y el flujo `--update` (firma RSA + self-test).
- El gate de aprobación local (`src/core/policy.c`, `src/core/command.c`, `src/app/session.c`).
- Manejo de API keys (`src/platform/win32/credential_win32.c`) y `config.ini`.
- El relay de reportes/telemetría (`relay/`).

Fuera de alcance: el proveedor del modelo, Windows mismo o PCs ya comprometidas.

## Reglas de operación segura

El detalle completo está en [`docs/SECURITY.md`](docs/SECURITY.md). En corto:

- El modelo es un recomendador no confiable: leé cada comando antes de aprobarlo.
- La seguridad está en el código local, no en el prompt.
- La API key solo se guarda si aceptás, en el Administrador de credenciales de Windows. `tinyshell logout` la borra.
- Nunca corras como administrador para tareas de rutina.
- Los updates nunca son automáticos y siempre verifican firma RSA antes de instalar.
