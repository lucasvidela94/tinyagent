# TinyShell Agent

Agente de diagnóstico y reparación asistida para **Noblex NT1013E / Windows 7 SP1 de 32 bits**, con recursos limitados. Está escrito en C y consulta un modelo remoto de OpenCode Go; los comandos se ejecutan localmente en `cmd.exe` y cada comando requiere aprobación en la consola.

> **Estado:** prototipo de código fuente. No se incluye un `.exe` precompilado ni se afirma que esté probado en el equipo destino. La compilación correcta no demuestra que TLS, autenticación, formato de herramientas y ejecución funcionen en Windows 7: hay que hacer pruebas reales.

## Perfil del equipo objetivo

- Equipo: Noblex NT1013E (nombre de host observado: `SAMIRA`).
- Sistema: Windows 7 Professional SP1, build 7601, arquitectura x86 / 32 bits.
- CPU: Intel Atom N2600 @ 1.60 GHz; 2 núcleos / 4 procesadores lógicos reportados.
- RAM informada: 2 GB.
- Restricciones de diseño: consumo local bajo, sin runtime de Go/Node/Python, sin base de datos, sin servicios residentes, sin descarga de modelos al equipo.
- Inferencia: remota. El consumo principal local es el proceso C y `cmd.exe`; la calidad/latencia dependen de Internet y del servicio API.

## Compilar desde Linux

En Linux Mint, Ubuntu o Debian:

```bash
sudo apt update
sudo apt install mingw-w64 file
```

Desde la raíz de esta carpeta:

```bash
chmod +x build/build-linux-mingw.sh
./build/build-linux-mingw.sh
```

El resultado esperado es `tinyshell.exe`, Windows x86. Alternativa manual:

```bash
i686-w64-mingw32-gcc -std=c99 -Os -s -Wall -Wextra -o tinyshell.exe agent.c -lwinhttp
```

## Compilar desde Windows

Con Visual Studio C++ Build Tools y un Windows SDK compatible, abrir **x86 Native Tools / Developer Command Prompt** y ejecutar:

```bat
cl /nologo /O1 /MT /W3 agent.c winhttp.lib /Fe:tinyshell.exe
```

También está `build/build-windows-msvc.bat`. Si el compilador instalado no soporta como objetivo Windows 7, el ejecutable puede compilar y aun así no funcionar en ese sistema antiguo.

## Ejecutar

1. Copiar `tinyshell.exe` a una carpeta local de la Noblex.
2. Abrir CMD normal (no como administrador para la primera prueba) y ejecutar `tinyshell.exe`.
3. Introducir la API key de OpenCode Go cuando la solicite. No se guarda en un archivo.
4. Escribir una tarea en español, por ejemplo: `Revisá la versión de Windows, memoria disponible y procesos con mayor consumo; no cambies nada`.
5. Leer la propuesta de comando. Solo se ejecuta si se responde `s`.
6. Salir con `/salir`.

## Modelo y API

- Proveedor: OpenCode Go.
- Modelo configurado en el código: `deepseek-v4.1-flash`.
- Endpoint configurado: `https://opencode.ai/zen/go/v1/chat/completions`.
- Referencia: <https://docs.opencode.ai/docs/go/>

La cuenta debe tener acceso al servicio/modelo y una API key válida. No se incluye ninguna clave en este repositorio.

## Límites actuales

- Una herramienta: ejecución de comandos CMD con salida devuelta al modelo.
- No tiene editor de archivos propio, explorador de proyecto, control de procesos interactivo, sandbox, rollback ni verificador formal de comandos.
- El filtro de comandos potencialmente peligrosos es heurístico y **no reemplaza el criterio del usuario**. Revisar cada comando completo antes de aprobarlo.
- El prototipo no debe tratarse como un agente autónomo con permisos irrestrictos.
- WinHTTP/TLS 1.2 y certificados raíz actualizados son requisitos prácticos para HTTPS en Windows 7. Si falla la conexión, comprobar primero fecha/hora, actualizaciones de certificados y compatibilidad TLS.
- La API puede cambiar o rechazar el formato de tool calling. Debe verificarse con una petición real y una tarea inocua antes de usarlo para reparar.

## Documentación del proyecto

- [`docs/GOALS.md`](docs/GOALS.md): objetivo y criterios de éxito.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): arquitectura y mapa del código.
- [`docs/ACCESS_POINTS.md`](docs/ACCESS_POINTS.md): puntos de entrada locales, red y configuración.
- [`docs/PC_PROFILE.md`](docs/PC_PROFILE.md): restricciones derivadas del hardware/sistema.
- [`docs/OPERATIONS.md`](docs/OPERATIONS.md): flujo de uso y diagnóstico inicial.
- [`docs/SECURITY.md`](docs/SECURITY.md): permisos, secretos y límites de seguridad.
- [`docs/API.md`](docs/API.md): contrato de integración remoto.
- [`docs/ROADMAP.md`](docs/ROADMAP.md): mejoras priorizadas.
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md): pruebas mínimas antes de confiar en el programa.
- [`scripts/collect-diagnostics.bat`](scripts/collect-diagnostics.bat): recopilación local básica sin instalar herramientas.

## Licencia / estado

Prototipo experimental. No usar para cambios críticos, datos únicos o reparaciones automáticas sin supervisión. Revisar y adaptar antes de distribuir.
