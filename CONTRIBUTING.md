# Cómo contribuir a TinyShell

Gracias por querer aportar. No hay burocracia: si la idea mejora el proyecto, es bienvenida.

## Formas de ayudar

- Probar en una PC real vieja (Windows 7 32 bits ideal) y contar qué pasó.
- Reportar errores con pasos para repetirlos.
- Proponer o mandar un fix pequeño.
- Mejorar docs, mensajes o el instalador.
- Agregar tests en `tests/` para casos que hoy no están cubiertos.

No hace falta pedir permiso antes: abrí el issue o el PR directamente.

## Antes de programar

1. Leé `docs/ARCHITECTURE.md`, `docs/SECURITY.md` y `docs/TEST_PLAN.md` si vas a tocar ejecución, red o parsing.
2. Mirá `AGENTS.md` (reglas del repo) y `GLOSSARY.md` (términos).
3. Si el cambio es grande, abrí primero un issue corto contando la idea para no trabajar de más.

## Puesta en marcha

```bash
# tests del núcleo portable
./build/test-core.sh

# cross-build Windows 32 bits (necesita mingw-w64)
./build/build-linux-mingw.sh

# todo lo que corre CI
./build/check.sh --build
```

En Windows, desde el **x86 Native Tools Command Prompt**: `build\build-windows-msvc.bat`.

`pnpm install` una vez activa el hook de pre-commit (husky), que corre `build/check.sh`. Node es solo herramienta de desarrollo, nunca requisito de runtime.

## Reglas del código

- Un commit hace una sola cosa, menos de ~400 líneas sin contar tests, directo a `main`.
- Cada archivo fuente ronda las ~200 líneas (máximo ~300). Si crece, se divide por interfaz de módulo real.
- Las llamadas a Windows viven en `src/platform/win32/`; `core` y `app` nunca incluyen un adaptador.
- Sin dependencias nuevas sin justificarlo en el mensaje de commit.
- Sin abstracciones de una sola implementación (nada de vtables/providers hasta tener dos casos reales).
- Cambio de comportamiento requiere test. Un refactor no cambia comportamiento.
- Comentarios que explican el *por qué*, nunca repiten el código.
- Prohibidos `gets/sprintf/strtok/system/popen/wsprintf/variantes-A/alloca`. `strcpy/strcat` solo con guarda de longitud en las líneas previas (`build/checks/banned.sh` lo verifica).
- Nunca edites `build/` o la config para que un check pase: se arregla el código o se discute el check.

## Commits y PRs

- Mensajes cortos en español o inglés, en presente: `agrega timeout a http_get`, no `agregado...`.
- Explicá el *por qué* del cambio y qué probaste (comando + resultado).
- Un PR = un tema. Mantenelo chico y fácil de revisar.
- La plantilla del PR te pide: qué/por qué/cómo, issue relacionado, tests y impacto en docs o releases. Completala, no la borres.
- CI debe pasar: checks estáticos, tests con sanitizers, cross-build Win32, presupuesto del `.exe` y tests de TUI.

## Tests que se esperan

- Casos de JSON malformado, campos faltantes, herramientas desconocidas, red caída, comandos rechazados y salidas gigantes.
- Si tocás `json` o `stream`, corré `./build/fuzz.sh`.
- No digas que algo anda en Windows 7 hasta probarlo en Windows 7 real (o aclarar que fue bajo wine).

## Reportar errores

Usá la plantilla de bug report: qué esperabas, qué pasó, versión (`tinyshell --version`), Windows, proveedor/modelo y pasos mínimos para repetirlo. Si hay un crash o un problema de seguridad, mirá `SECURITY.md`.

## Preguntas

Abrí un issue o una discussion con tu duda. No hay preguntas tontas en este repo.
