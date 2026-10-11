# Goals and non-goals

## Goal

Proveer un agente de terminal pequeño y de propósito general (idea similar a OpenCode o Pi) para PCs viejas con Windows: diagnosticar, inspeccionar archivos, escribir scripts, mantenimiento o responder preguntas sobre la máquina. El modelo razona remotamente; la máquina vieja solo mantiene la consola, realiza HTTPS y ejecuta comandos que el usuario aprueba. El equipo de prueba principal es Windows 7 x86.

## Goals

1. Ejecutable Win32/x86 compatible con el equipo objetivo, con consumo local bajo.
2. No depender de Node.js, Go, Python, una base de datos ni un servicio en segundo plano al ejecutar el binario.
3. Hablar con cualquier API compatible con OpenAI chat completions, configurable en `config.ini` (por defecto OpenCode Go con `deepseek-v4.1-flash`).
4. Mostrar comando exacto y motivo antes de ejecutarlo.
5. Inspeccionar primero; cambios solo después de una aprobación consciente.
6. Entregar salida y código de retorno al modelo para iterar sobre evidencia real.
7. Mantener el proyecto fácil de compilar, inspeccionar y modificar.
8. Mantener el system prompt agnóstico de tarea y de equipo: la máquina se detecta al arrancar.

## Non-goals (por ahora)

- Ejecutar tareas sin supervisión o elevar privilegios automáticamente.
- Sustituir un antivirus, herramienta de recuperación, técnico o copia de seguridad.
- Ejecutar un LLM local en 2 GB de RAM.
- Ser un clon completo de Pi/OpenCode: no hay navegación de codebase, edición integrada, git tools, MCP, shell persistente o planificación avanzada.
- Garantizar reparación correcta en todos los casos.

## Definition of done for the first usable build

- Compila como PE32 x86 y se identifica como 32 bits.
- Arranca en Windows 7 SP1 x86 sin instalar runtimes adicionales.
- Una petición de diagnóstico inocua completa la ida y vuelta con la API.
- La respuesta puede proponer una herramienta; rechazarla evita toda ejecución.
- Aprobar un comando de solo lectura devuelve stdout/stderr y el resultado al modelo.
- Clave API no escrita a disco ni impresa en logs.
- Casos de error de red/API se muestran sin cerrar inesperadamente el programa.
