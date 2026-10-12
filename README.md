# TinyShell

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/lucasvidela94/tinyagent)](https://github.com/lucasvidela94/tinyagent/releases/latest)
[![CI](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml/badge.svg)](https://github.com/lucasvidela94/tinyagent/actions/workflows/ci.yml)
[![Windows 7+ 32-bit](https://img.shields.io/badge/Windows-7%2B%2032--bit-blue)](docs/PC_PROFILE.md)
[![Single exe ~70 KB](https://img.shields.io/badge/exe-%7E70KB-lightgrey)](docs/ARCHITECTURE.md)

Agente de terminal de propósito general (en la línea de OpenCode o Pi) para **PCs viejas con Windows y pocos recursos**: diagnosticar problemas, inspeccionar archivos, escribir scripts, hacer mantenimiento o responder preguntas sobre la máquina.

- Un solo `.exe` de 32 bits en C (~70 KB). Sin Node, Python, .NET, servicios ni base de datos.
- El modelo corre remoto, en cualquier API compatible con OpenAI (por defecto OpenCode Go); la PC solo mantiene la consola, habla HTTPS y ejecuta comandos.
- **Cada comando lo aprobás vos** antes de que se ejecute. Los riesgosos piden además escribir `EJECUTAR`.
- Equipo de prueba principal: Noblex NT1013E, Windows 7 SP1 32 bits, Atom N2600, 2 GB RAM.

Sitio y descargas: **https://tinyshell.lucasan-videla.workers.dev**

> **Estado:** prototipo. Compila y pasa los tests en Linux, pero todavía **no se probó en el equipo Windows 7 real**. Ver [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## Contenido

- [Instalar](#instalar) · [Usar](#usar) · [Proveedor y modelo](#proveedor-y-modelo)
- [Reportar un problema](#reportar-un-problema) · [Actualizar y volver atrás](#actualizar-y-volver-atrás)
- [Seguridad](#seguridad-en-corto) · [Compilar desde el código](#compilar-desde-el-código) · [Documentación](#documentación)
- [Cómo contribuir](#cómo-contribuir) · [Soporte](#soporte) · [Licencia](#licencia)

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
- Una API key de [OpenCode Go](https://docs.opencode.ai/docs/go/) u otro proveedor (ver "Proveedor y modelo").

## Usar

Primero iniciá sesión (una sola vez):

```text
C:\> tinyshell login
Elegí un proveedor (flechas y Enter):
> opencode-go    OpenCode Go (el modelo se elige después)

API key de opencode-go (clic derecho para pegar): ****************************

Elegí un modelo de opencode-go (38; escribí para filtrar, Enter):
  filtro: kimi_
> kimi-k2.6
  kimi-k2.7-code
  kimi-k3
```

La key se verifica contra el proveedor antes de guardarla (si la rechaza, no se guarda nada) y queda cifrada en el Administrador de credenciales de Windows. La lista de modelos se pide en vivo, así que aparece cualquier modelo nuevo del proveedor. `tinyshell logout` borra la key.

Después abrí una ventana **nueva** de CMD y escribí `tinyshell`. Es una conversación: pedís algo, el agente va paso a paso proponiendo comandos que aprobás, hasta terminar.

```text
TinyShell 0.6.0 · opencode-go · /ayuda

 qué programas arrancan con Windows?                    <- tu mensaje

$ wmic startup get caption,command                      <- propuesta (amarillo; rojo si es riesgosa)
  ver programas que inician con Windows
  ejecutar? [s/N] s
  Adobe ARM   "C:\Program Files\...\reader_sl.exe"
  ... +18 líneas · exit=0

Tenés 23 programas al inicio. Los que más pesan son...
─────────────────────────────────────────────────────────
> _
─────────────────────────────────────────────────────────
C:\Users\Samira · 7.2k in · 0.6k out · 12%    deepseek-v4.1-flash
```

| Comando | Qué hace |
|---|---|
| `/ayuda` | muestra los comandos |
| `/modelo [x]` | elegir modelo de la lista (o escribir el nombre) |
| `/reporte` | contar un problema a los desarrolladores |
| `/login` | cambiar de proveedor, key o modelo |
| `/reiniciar` | empieza una conversación nueva |
| `/actualizar` | igual que `tinyshell --update` |
| `/salir` | termina |

**Ctrl+C** cancela lo que esté pasando (la respuesta del modelo o el comando en curso) sin cerrar TinyShell; en el prompt vacío, sale. En el editor funcionan las teclas de siempre del CMD: flechas para mensajes anteriores, F7 para el historial, clic derecho para pegar. Límites para no ahogar una máquina vieja: cada comando corre sin entrada interactiva y se corta a los 180 s; la salida que ve el modelo se recorta a 24 KB; el historial a ~192 KB.

## Proveedor y modelo

Por defecto usa OpenCode Go. Para usar otro servicio compatible con OpenAI, editá `%LOCALAPPDATA%\TinyShell\config.ini` con el Bloc de notas (se crea con ejemplos la primera vez):

```ini
[provider]
name = openrouter
url = https://openrouter.ai/api/v1/chat/completions
model = deepseek/deepseek-chat
session_header =
```

- `name` identifica la API key guardada: cada proveedor tiene la suya.
- `session_header` es solo para OpenCode Go (`x-opencode-session`); dejalo vacío para los demás.
- Un servidor en tu red (LM Studio, Ollama en otra PC) funciona con `http://` y sin key; TinyShell avisa que esa conexión no va cifrada.
- `/modelo` muestra la lista de modelos del proveedor para elegir; `/modelo otro-modelo` lo cambia directo. Se guarda en `config.ini`.

## Reportar un problema

Si algo falla, escribí `/reporte` dentro de TinyShell (o `tinyshell reporte` en CMD):

1. contás en una línea qué pasó;
2. TinyShell arma el informe (versión, Windows, modelo, últimos errores y, si aceptás, la conversación), lo guarda en `%LOCALAPPDATA%\TinyShell\reportes\` y **te muestra todo lo que se va a enviar**;
3. solo si respondés `s` se envía: se abre un issue público en GitHub con un resumen sin datos personales, y el informe completo queda privado para el desarrollador.

Si no hay internet, mandá el archivo guardado a lucasan.videla@gmail.com.

### Telemetría (beta)

Mientras TinyShell esté en beta (versiones 0.x) envía datos técnicos para encontrar errores antes de que los reportes: versión, Windows, modelo, cantidad de pasos y aprobaciones, duración y códigos de error. **Nunca** lo que escribís, los comandos ni sus salidas. Se avisa la primera vez.

```bat
tinyshell telemetria --ver   :: lo que se envió
tinyshell telemetria off     :: apagarla
```

## Actualizar y volver atrás

```bat
tinyshell --update     :: busca, pregunta, verifica la firma e instala
tinyshell --rollback   :: vuelve a la versión anterior
tinyshell --version
tinyshell logout       :: borra la API key guardada
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
- La API key solo se guarda si decís que sí, en el Administrador de credenciales de Windows. `tinyshell logout` la borra.
- No lo ejecutes como administrador para tareas de rutina.
- Lo que escribís y la salida de los comandos se envían al proveedor del modelo.

Vulnerabilidades: escribí a lucasan.videla@gmail.com, no abras issue público. Detalle en [`SECURITY.md`](SECURITY.md) y [`docs/SECURITY.md`](docs/SECURITY.md).

## Compilar desde el código

En Linux con MinGW-w64 (`sudo apt install mingw-w64` / `sudo pacman -S mingw-w64-gcc`):

```bash
./build/test-core.sh          # tests del núcleo portable (JSON, historial, política, versiones)
./build/build-linux-mingw.sh  # genera tinyshell.exe (PE32, enlaza msvcrt.dll, no la UCRT)
./build/check.sh --build      # todos los chequeos + build (lo mismo que corre CI)
./build/fuzz.sh               # fuzz de los parsers JSON, SSE y HTTP (clang con libFuzzer)
./build/build-linux-musl.sh   # binario Linux estático para i586 (necesita Docker)
```

Para contribuir: `pnpm install` una vez activa el hook de pre-commit (husky), que corre
`build/check.sh`. Node es solo herramienta de desarrollo: TinyShell no lo necesita para
correr. El código está ordenado por capas (`src/core`, `src/app`, `src/platform`); ver
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

En Windows, desde el **x86 Native Tools Command Prompt** de Visual Studio: `build\build-windows-msvc.bat`.

Un binario compilado por vos no trae la clave de releases salvo que generes la tuya, así que `--update` no va a funcionar con él; el resto sí.

## Publicar un release (mantenedor)

Una sola vez: `./build/gen-signing-key.sh` crea la clave privada en `~/.config/tinyshell/` (hacé un backup offline; nunca va al repo) y escribe la pública en `keys/` y `src/release_key.h` (esas sí se commitean).

Cada release, desde `main` limpio:

1. Subí `TINYSHELL_VERSION` en `src/config.h` y agregá `## X.Y.Z` al `CHANGELOG.md`; commit.
2. `./build/release.sh X.Y.Z --dry-run` para revisar los artefactos en `dist/`.
3. `./build/release.sh X.Y.Z`: corre los tests, compila, firma, crea el tag y publica con `gh`.

Ver [`CHANGELOG.md`](CHANGELOG.md) para el historial.

## Cómo contribuir

¡Todo aporte suma! No hay código de conducta formal: mientras quieras aportar de buena fe, estás bienvenido/a.

1. Leé [`CONTRIBUTING.md`](CONTRIBUTING.md) (5 minutos).
2. Para errores usá la [plantilla de bug](.github/ISSUE_TEMPLATE/bug_report.yml); para ideas, la [de feature](.github/ISSUE_TEMPLATE/feature_request.yml).
3. Un PR chico con tests pasa rápido. La [plantilla de PR](.github/PULL_REQUEST_TEMPLATE.md) te guía.

## Soporte

- ¿Duda de uso? [Discussions](https://github.com/lucasvidela94/tinyagent/discussions).
- ¿Error? [Bug report](https://github.com/lucasvidela94/tinyagent/issues/new/choose).
- Sin internet: `scripts/collect-diagnostics.bat` y el archivo a lucasan.videla@gmail.com.
- Detalle en [`SUPPORT.md`](SUPPORT.md).

## Documentación

- [`docs/GOALS.md`](docs/GOALS.md): objetivo y criterios de éxito.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): capas, módulos y flujo.
- [`docs/adr/`](docs/adr/): decisiones de arquitectura (port a Linux).
- [`GLOSSARY.md`](GLOSSARY.md): términos del dominio.
- [`docs/SYSTEM_PROMPT.md`](docs/SYSTEM_PROMPT.md): cómo está escrito el prompt del sistema.
- [`docs/ACCESS_POINTS.md`](docs/ACCESS_POINTS.md): entradas locales, red y comandos.
- [`docs/SECURITY.md`](docs/SECURITY.md): modelo de confianza y límites.
- [`docs/OPERATIONS.md`](docs/OPERATIONS.md): instalación, problemas de TLS, diagnóstico.
- [`docs/API.md`](docs/API.md): contrato con el proveedor del modelo.
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) y [`docs/ROADMAP.md`](docs/ROADMAP.md).
- [`docs/PC_PROFILE.md`](docs/PC_PROFILE.md): el equipo de prueba.
- [`scripts/collect-diagnostics.bat`](scripts/collect-diagnostics.bat): informe local sin API.

## Licencia

[MIT](LICENSE).
