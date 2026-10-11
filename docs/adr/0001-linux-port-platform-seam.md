# ADR 0001: port a Linux con un seam de plataforma

Estado: aceptado (2026-10-11). Preguntas abiertas al final.

## Contexto

TinyShell existe para revivir PCs viejas, primero las más viejas. Muchas de ellas corren
distros Linux de 32 bits, no Windows. Hoy todo el código del sistema es Win32 y los
archivos `*_win32.c` mezclan dos cosas:

- adaptadores de verdad (consola, WinHTTP, CreateProcess, Credential Manager, config.ini,
  Ctrl+C, CryptoAPI);
- lógica portable con nombre de Windows: `tui`, `tui_select` y `login` no usan Win32;
  `session` y `main` solo usan reloj, random y carpeta actual; `update` mezcla el flujo
  (versión, firma, self-test, swap) con el renombrado de archivos.

También hay primitivas repetidas en varios módulos (random, reloj, leer/escribir archivos,
carpeta actual, nombre de PC y usuario) e interfaces que dejan ver Windows
(`settings_file(wchar_t *)`, `http_last_error()` "código WinHTTP",
`cancel_watch_process(job, process)`).

## Decisión

Un **seam de plataforma** con dos adaptadores: Win32 (el actual) y POSIX. Con Linux el
seam deja de ser hipotético: hay dos adaptadores reales.

```
src/          núcleo portable + los .h, que son las interfaces del seam (una sola definición)
src/win32/    adaptadores Windows
src/posix/    adaptadores Linux
vendor/       librería TLS (solo posix)
```

- Sin `#ifdef` de plataforma en el núcleo: cada script de build elige su carpeta.
- `sys.h` (nuevo): random, reloj monótono, carpeta actual, carpeta de datos, archivos con
  rutas UTF-8, nombre de PC y usuario. Junta las primitivas repetidas.
- **Dialecto de shell** como datos: reglas de riesgo de `policy` y guía de shell del prompt,
  una tabla para `cmd.exe` y otra para `/bin/sh`. Son datos puros: los tests prueban los dos
  en el host.
- `update` portable: el flujo se escribe una vez; la plataforma aporta su ruta, renombrar y
  ejecutarse con `--version`.
- `cancel.h` público queda en `init`, `requested`, `reset` y `take_input_interrupt`; vigilar
  un request o un proceso es privado de cada plataforma.

Linux:

- Binario **estático de 32 bits** con musl, compilado en Docker (`i386/alpine`), CPU base
  i586. No depende de la glibc ni del OpenSSL de la distro.
- **TLS empaquetado: Mbed TLS 4.1 LTS** (soporte hasta 2029-03). Trae RSA y SHA-256, así que
  también verifica la firma de los updates. Certificados: primero el bundle del sistema,
  después uno de Mozilla embebido que se renueva en cada release (las distros viejas
  traen certificados vencidos).
- Terminal con termios y escapes ANSI, glifos ASCII-safe. Shell con `fork`/`exec` de
  `/bin/sh -c` en su propio grupo de procesos (timeout y Ctrl+C matan el árbol).
- Config en `~/.config/tinyshell/`, binario en `~/.local/bin`, `install.sh` para curl/wget.

## Orden

1. Paso 0, solo Windows y sin cambiar comportamiento: renombrar los módulos mal
   etiquetados, `sys.h`, limpiar las interfaces que dejan ver Windows, dialecto de shell,
   `update` portable y mover los adaptadores a `src/win32/`. Mismo binario, mismos tests.
2. Adaptadores posix: sys, consola, shell y cancel; después HTTP con TLS; al final
   credenciales y update.
3. Release dual (`tinyshell.exe` y `tinyshell-linux-i386`, firmados) y sitio.

## Alternativas descartadas

- **TLS del sistema** (OpenSSL dinámico o libcurl): en las distros viejas es 0.9.8/1.0.1, con
  ABI distinta por distro y certificados vencidos.
- **BearSSL**: más chico y auditable, pero sigue en beta y su último release es 0.6 (2018).
- **`#ifdef _WIN32` dentro de cada módulo**: mezcla las dos plataformas en cada archivo y
  rompe la locality.

## Preguntas abiertas

- Dónde guardar la API key en Linux (no hay llavero común en distros viejas): archivo
  `0600` con opt-in, o no guardarla. Cualquiera cambia la regla de `AGENTS.md`.
- Piso de hardware (¿i586 alcanza?) y distros concretas para probar. Docker prueba
  userlands viejos, no kernels viejos: eso pide QEMU o la PC real.
