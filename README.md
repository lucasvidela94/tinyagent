# TinyShell

Agente de terminal de propósito general (en la línea de OpenCode o Pi) para **PCs viejas con Windows y pocos recursos**: diagnosticar problemas, inspeccionar archivos, escribir scripts, hacer mantenimiento o responder preguntas sobre la máquina.

- Un solo `.exe` de 32 bits en C (~70 KB). Sin Node, Python, .NET, servicios ni base de datos.
- El modelo corre remoto (OpenCode Go, `deepseek-v4.1-flash`); la PC solo mantiene la consola, habla HTTPS y ejecuta comandos.
- **Cada comando lo aprobás vos** antes de que se ejecute. Los riesgosos piden además escribir `EJECUTAR`.
- Equipo de prueba principal: Noblex NT1013E, Windows 7 SP1 32 bits, Atom N2600, 2 GB RAM.

> **Estado:** prototipo. Compila y pasa los tests en Linux, pero todavía **no se probó en el equipo Windows 7 real**. Ver [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## Instalar

Abrí **CMD** (no como administrador) y pegá la línea que corresponda. Instala en `%LOCALAPPDATA%\TinyShell` y lo agrega a tu `PATH`.

**Windows 10 / 11** (traen `curl`):

```bat
curl -fsSLo "%TEMP%\tinyshell-install.bat" https://github.com/lucasvidela94/tinyagent/releases/latest/download/install.bat && "%TEMP%\tinyshell-install.bat"
```

**Windows 7** (no trae `curl`; usa `bitsadmin`):

```bat
bitsadmin /transfer tinyshell /download /priority foreground https://github.com/lucasvidela94/tinyagent/releases/latest/download/install.bat "%TEMP%\tinyshell-install.bat" && "%TEMP%\tinyshell-install.bat"
```

`bitsadmin` necesita TLS 1.2 habilitado para todo el sistema (GitHub lo exige), algo que muchos Windows 7 no tienen. Si falla, **instalá por USB**:

1. En otra PC, bajá `tinyshell.exe` e `install.bat` de la [página de releases](https://github.com/lucasvidela94/tinyagent/releases/latest).
2. Copialos **juntos** a un pendrive.
3. En la PC vieja, ejecutá `install.bat` desde el pendrive: instala la copia local sin descargar nada.

El instalador muestra el SHA-256 del ejecutable; comparalo con el que figura en el release.

### Requisitos en Windows 7

- Service Pack 1 y la actualización **KB3140245** (TinyShell pide TLS 1.2 por su cuenta; sin ella no puede conectarse).
- Certificados raíz actualizados y fecha/hora correctas.
- Una API key de [OpenCode Go](https://docs.opencode.ai/docs/go/).

## Usar

Abrí una ventana **nueva** de CMD y escribí `tinyshell`. Te pide la API key (no se muestra ni se guarda en disco) y después:

```text
vos> revisá cuánta RAM libre hay y qué procesos consumen más; no cambies nada

[PROPUESTA] tasklist /fo table /nh
Motivo: ver procesos y memoria en uso
Ejecutar? [s/N]: s
```

| Comando | Qué hace |
|---|---|
| `/salir` | termina |
| `/reiniciar` | borra el historial de la conversación |
| `/actualizar` | igual que `tinyshell --update` |

Límites para no ahogar una máquina vieja: cada comando corre sin entrada interactiva y se corta a los 180 s; la salida que ve el modelo se recorta a 24 KB; el historial a ~192 KB.

## Actualizar y volver atrás

```bat
tinyshell --update     :: busca, pregunta, verifica la firma e instala
tinyshell --rollback   :: vuelve a la versión anterior
tinyshell --version
```

Nunca se actualiza solo. `--update` funciona aunque el agente esté roto (no pide API key) y:

1. compara tu versión con la del último release;
2. pregunta antes de bajar nada;
3. verifica la **firma RSA** del ejecutable con la clave pública que trae el binario: un release que no esté firmado con la clave del proyecto se rechaza, aunque venga de GitHub;
4. corre el binario nuevo con `--version` en **tu** equipo: si no arranca en ese Windows, no se instala;
5. guarda el anterior como `tinyshell.old.exe` para `--rollback`.

Si algo sale muy mal, volver a correr el instalador (o el USB) siempre deja una instalación limpia.

## Seguridad, en corto

- El modelo es un recomendador no confiable. Leé cada comando completo antes de aprobarlo.
- La seguridad está en el código local, no en el prompt: herramienta desconocida, comando vacío, con saltos de línea o de más de 2000 bytes se rechazan antes de preguntarte.
- La detección de comandos riesgosos es una heurística, no una barrera: tu aprobación es el control real.
- No lo ejecutes como administrador para tareas de rutina.
- Lo que escribís y la salida de los comandos se envían al proveedor del modelo.

Detalle completo en [`docs/SECURITY.md`](docs/SECURITY.md).

## Compilar desde el código

En Linux con MinGW-w64 (`sudo apt install mingw-w64` / `sudo pacman -S mingw-w64-gcc`):

```bash
./build/test-core.sh          # tests del núcleo portable (JSON, historial, política, versiones)
./build/build-linux-mingw.sh  # genera tinyshell.exe (PE32, enlaza msvcrt.dll, no la UCRT)
```

En Windows, desde el **x86 Native Tools Command Prompt** de Visual Studio: `build\build-windows-msvc.bat`.

Un binario compilado por vos no trae la clave de releases salvo que generes la tuya, así que `--update` no va a funcionar con él; el resto sí.

## Publicar un release (mantenedor)

Una sola vez: `./build/gen-signing-key.sh` crea la clave privada en `~/.config/tinyshell/` (hacé un backup offline; nunca va al repo) y escribe la pública en `keys/` y `src/release_key.h` (esas sí se commitean).

Cada release, desde `main` limpio:

1. Subí `TINYSHELL_VERSION` en `src/config.h` y agregá `## X.Y.Z` al `CHANGELOG.md`; commit.
2. `./build/release.sh X.Y.Z --dry-run` para revisar los artefactos en `dist/`.
3. `./build/release.sh X.Y.Z`: corre los tests, compila, firma, crea el tag y publica con `gh`.

## Documentación

- [`docs/GOALS.md`](docs/GOALS.md): objetivo y criterios de éxito.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): módulos y flujo.
- [`docs/ACCESS_POINTS.md`](docs/ACCESS_POINTS.md): entradas locales, red y comandos.
- [`docs/SECURITY.md`](docs/SECURITY.md): modelo de confianza y límites.
- [`docs/OPERATIONS.md`](docs/OPERATIONS.md): instalación, problemas de TLS, diagnóstico.
- [`docs/API.md`](docs/API.md): contrato con el proveedor del modelo.
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) y [`docs/ROADMAP.md`](docs/ROADMAP.md).
- [`docs/PC_PROFILE.md`](docs/PC_PROFILE.md): el equipo de prueba.
- [`scripts/collect-diagnostics.bat`](scripts/collect-diagnostics.bat): informe local sin API.
