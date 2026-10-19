/* TABLE: Spanish translations, keyed by the English source text (see i18n.h). Checked against the
 * code by build/checks/i18n.sh: every tr()/N_() string needs an entry here with the same
 * printf conversions. Rioplatense Spanish ("vos"), as the target users speak. */
#include "i18n.h"

const I18nPair I18N_ES[] = {
    {"  (type to filter)",
     "  (escribí para filtrar)"},
    {"  ... +%d more",
     "  ... +%d más"},
    {"  filter: %s_",
     "  filtro: %s_"},
    {"  risk: ",
     "  riesgo: "},
    {"  run it? [y/N] ",
     "  ¿ejecutar? [s/N] "},
    {"  type RUN to confirm: ",
     "  escribí EJECUTAR para confirmar: "},
    {" (invalid API key or no access to the model)",
     " (API key inválida o sin acceso al modelo)"},
    {" (not found; is there a published release?)",
     " (no encontrado; ¿hay un release publicado?)"},
    {" (provider problem; try again later)",
     " (problema del proveedor; reintentá más tarde)"},
    {" (usage limit; wait and try again)",
     " (límite de uso; esperá y reintentá)"},
    {"%s \xc2\xb7 %s in \xc2\xb7 %s out \xc2\xb7 %d%%",
     "%s \xc2\xb7 %s entrada \xc2\xb7 %s salida \xc2\xb7 %d%%"},
    {"(answer cut off by the token limit)",
     "(respuesta cortada por el límite de tokens)"},
    {"(empty answer)",
     "(respuesta vacía)"},
    {"(no matches)",
     "(sin coincidencias)"},
    {"(no output)",
     "(sin salida)"},
    {"(no reason given)",
     "(sin motivo)"},
    {". Everything that is sent is in:\n",
     ". Todo lo que se envía está en:\n"},
    {"... +%d lines",
     "... +%d líneas"},
    {"/model [x]  pick a model from the list (or type its name)\n"
     "/login      change provider or API key\n"
     "/report     tell the developers about a problem\n"
     "/reset      start a new conversation\n"
     "/update     check for and install the latest version\n"
     "/quit       quit",
     "/model [x]  elegir modelo de la lista (o escribir el nombre)\n"
     "/login      cambiar de proveedor o de API key\n"
     "/report     contar un problema a los desarrolladores\n"
     "/reset      empezar una conversación nueva\n"
     "/update     buscar e instalar la última versión\n"
     "/quit       salir"},
    {"; TinyShell: the model provider. No secrets here: API keys go to the Windows\r\n"
     "; Credential Manager (one per 'name'). Any OpenAI-compatible chat-completions service.\r\n"
     ";\r\n"
     "; Examples:\r\n"
     ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\r\n"
     ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\r\n"
     ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\r\n"
     ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: the key travels unencrypted)\r\n"
     "; For providers other than OpenCode Go, leave session_header empty.\r\n"
     "; [ui] language: es, en or auto (the system's language).\r\n",
     "; TinyShell: proveedor del modelo. Sin secretos: las API keys van al Administrador de\r\n"
     "; credenciales de Windows (una por 'name'). Cualquier servicio compatible con OpenAI chat completions.\r\n"
     ";\r\n"
     "; Ejemplos:\r\n"
     ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\r\n"
     ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\r\n"
     ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\r\n"
     ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: la key viaja sin cifrar)\r\n"
     "; Para proveedores que no son OpenCode Go, dejá session_header vacío.\r\n"
     "; [ui] language: es, en o auto (el idioma del sistema).\r\n"},
    {"; TinyShell: the model provider. No secrets here: API keys go to\n"
     "; ~/.config/tinyshell/credentials (mode 0600, one per 'name'). Any OpenAI-compatible\n"
     "; chat-completions service.\n"
     ";\n"
     "; Examples:\n"
     ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\n"
     ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\n"
     ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\n"
     ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: the key travels unencrypted)\n"
     "; For providers other than OpenCode Go, leave session_header empty.\n"
     "; [ui] language: es, en or auto (the system's language).\n",
     "; TinyShell: proveedor del modelo. Sin secretos: las API keys van a\n"
     "; ~/.config/tinyshell/credentials (permisos 0600, una por 'name'). Cualquier servicio\n"
     "; compatible con OpenAI chat completions.\n"
     ";\n"
     "; Ejemplos:\n"
     ";   OpenAI      url = https://api.openai.com/v1/chat/completions      model = gpt-4o-mini\n"
     ";   OpenRouter  url = https://openrouter.ai/api/v1/chat/completions   model = deepseek/deepseek-chat\n"
     ";   DeepSeek    url = https://api.deepseek.com/chat/completions        model = deepseek-chat\n"
     ";   LM Studio   url = http://192.168.0.10:1234/v1/chat/completions    (http: la key viaja sin cifrar)\n"
     "; Para proveedores que no son OpenCode Go, dejá session_header vacío.\n"
     "; [ui] language: es, en o auto (el idioma del sistema).\n"},
    {"A new version is available: %s. Install it? [y/N]: ",
     "Hay una versión nueva: %s. ¿Instalarla? [s/N]: "},
    {"API key for %s (%s; Enter if none): ",
     "API key de %s (%s; Enter si no usa): "},
    {"API key for %s deleted.\n",
     "API key de %s borrada.\n"},
    {"Beta: TinyShell sends technical usage and error data (version, OS, model, error codes;\n"
     "never what you type or the commands). See it: tinyshell telemetry --show \xc2\xb7\n"
     "turn it off: tinyshell telemetry off",
     "Beta: TinyShell envía datos técnicos de uso y errores (versión, sistema, modelo, códigos de\n"
     "error; nunca lo que escribís ni los comandos). Ver: tinyshell telemetry --show \xc2\xb7\n"
     "apagar: tinyshell telemetry off"},
    {"Cancelled by the user with Ctrl+C. Nothing was run.",
     "Cancelado por el usuario con Ctrl+C. No se ejecutó nada."},
    {"Cancelled for safety. Nothing was run.",
     "Cancelado por seguridad. No se ejecutó nada."},
    {"Cancelled.\n",
     "Cancelado.\n"},
    {"Command too long. Nothing was run.",
     "Comando demasiado largo. No se ejecutó nada."},
    {"Could not create the output pipe. Nothing was run.",
     "No se pudo crear el pipe de salida. No se ejecutó nada."},
    {"Could not delete the API key for %s.\n",
     "No se pudo borrar la API key de %s.\n"},
    {"Could not install the new binary (error %lu); the previous one was restored.\n",
     "No se pudo instalar el binario nuevo (error %lu); se restauró el anterior.\n"},
    {"Could not locate the current executable.\n",
     "No se pudo ubicar el ejecutable actual.\n"},
    {"Could not move the current binary aside (error %lu). Nothing was installed.\n",
     "No se pudo apartar el binario actual (error %lu). No se instaló nada.\n"},
    {"Could not move the current binary aside (error %lu).\n",
     "No se pudo apartar el binario actual (error %lu).\n"},
    {"Could not restore the previous version (error %lu).\n",
     "No se pudo restaurar la versión anterior (error %lu).\n"},
    {"Could not send it. Email the file above to %s",
     "No se pudo enviar. Mandá el archivo de arriba a %s"},
    {"Could not start /bin/sh. Nothing was run.",
     "No se pudo iniciar /bin/sh. No se ejecutó nada."},
    {"Could not start cmd.exe (error %lu). Nothing was run.",
     "No se pudo iniciar cmd.exe (error %lu). No se ejecutó nada."},
    {"Could not write the new binary (is the folder read-only?).\n",
     "No se pudo escribir el binario nuevo (¿carpeta sin permiso de escritura?).\n"},
    {"Ctrl+Shift+V or middle-click to paste",
     "Ctrl+Shift+V o clic del medio para pegar"},
    {"Downloaded: %lu bytes, SHA-256 %s\n",
     "Descargado: %lu bytes, SHA-256 %s\n"},
    {"INVALID SIGNATURE: the binary is not from the release key. Nothing was installed.\n",
     "FIRMA INVÁLIDA: el binario no viene de la clave de releases. No se instaló nada.\n"},
    {"Include this session's conversation (it helps a lot to fix it)? [Y/n] ",
     "¿Incluir la conversación de esta sesión (ayuda mucho a arreglarlo)? [S/n] "},
    {"Invalid JSON answer. Nothing was run.",
     "Respuesta JSON inválida. No se ejecutó nada."},
    {"Last events sent (all of them in %s):\n"
     "%s",
     "Últimos eventos enviados (todos en %s):\n"
     "%s"},
    {"Limit of %d steps for this message. Type \"continue\" to go on.",
     "Límite de %d pasos para este mensaje. Escribí \"seguí\" para continuar."},
    {"Message too long; it was not sent.",
     "Mensaje demasiado largo; no se envió."},
    {"Nothing has been sent yet.\n",
     "Todavía no se envió nada.\n"},
    {"OpenCode Go (pick the model next)",
     "OpenCode Go (el modelo se elige después)"},
    {"Pick a model from %s (%d; type to filter, Enter):",
     "Elegí un modelo de %s (%d; escribí para filtrar, Enter):"},
    {"Pick a provider (arrows and Enter):",
     "Elegí un proveedor (flechas y Enter):"},
    {"Previous version restored. Close TinyShell and open it again.\n",
     "Versión anterior restaurada. Cerrá y volvé a abrir TinyShell.\n"},
    {"Public summary (published as a GitHub issue):",
     "Resumen público (se publica como issue en GitHub):"},
    {"RUN",
     "EJECUTAR"},
    {"Save it in %s? [y/N] ",
     "¿Guardarla en %s? [s/N] "},
    {"Send it to the developers? [y/N] ",
     "¿Enviar a los desarrolladores? [s/N] "},
    {"Signature verified.\n",
     "Firma verificada.\n"},
    {"TLS failed while negotiating the secure connection",
     "falló TLS al negociar la conexión segura"},
    {"TLS failed: check TLS 1.2 (KB3140245), root certificates and the date and time",
     "falló TLS: revisá TLS 1.2 (KB3140245), los certificados raíz y la fecha y hora"},
    {"Telemetry off.\n",
     "Telemetría desactivada.\n"},
    {"Telemetry on. Thanks.\n",
     "Telemetría activada. Gracias.\n"},
    {"Telemetry: %s \xc2\xb7 installation id %s\n"
     "Usage: tinyshell telemetry on | off | --show\n",
     "Telemetría: %s \xc2\xb7 id de instalación %s\n"
     "Uso: tinyshell telemetry on | off | --show\n"},
    {"The API key is missing.",
     "Falta la API key."},
    {"The API key was rejected. /login to change it.",
     "La API key fue rechazada. /login para cambiarla."},
    {"The answer has no message.",
     "La respuesta no trae message."},
    {"The command has an invalid encoding. Nothing was run.",
     "El comando tiene una codificación inválida. No se ejecutó nada."},
    {"The command has line breaks or control characters. Nothing was run.",
     "El comando tiene saltos de línea o caracteres de control. No se ejecutó nada."},
    {"The command is missing (no arguments or invalid JSON). Nothing was run.",
     "Faltó el comando (argumentos ausentes o JSON inválido). No se ejecutó nada."},
    {"The full report is also sent, privately (%lu KB)%s",
     "Además se envía el informe completo, privado (%lu KB)%s"},
    {"The new binary failed the start-up test on this machine. Nothing was installed.\n",
     "El binario nuevo no pasó la prueba de arranque en este equipo. No se instaló nada.\n"},
    {"The release's version.txt is invalid.\n",
     "El version.txt del release es inválido.\n"},
    {"The user rejected it. Nothing was run.",
     "El usuario lo rechazó. No se ejecutó nada."},
    {"There is no previous version saved (%s).\n",
     "No hay versión anterior guardada (%s).\n"},
    {"This binary has no release public key: it cannot verify updates.\n",
     "Este binario no trae la clave pública de releases: no puede verificar actualizaciones.\n"},
    {"TinyShell %s. Checking for updates...\n",
     "TinyShell %s. Buscando actualizaciones...\n"},
    {"Tool not allowed. Nothing was run.",
     "Herramienta no permitida. No se ejecutó nada."},
    {"Unknown command. /help lists the commands.",
     "Comando desconocido. /help muestra los comandos."},
    {"Unknown option: %s\n",
     "Opción desconocida: %s\n"},
    {"Updated to %s. Close TinyShell and open it again.\n"
     "If something fails: tinyshell --rollback\n",
     "Actualizado a %s. Cerrá y volvé a abrir TinyShell.\n"
     "Si algo falla: tinyshell --rollback\n"},
    {"Usage: tinyshell [login | logout | report | telemetry | --version | --update | --rollback | --help]\n"
     "  (no arguments)  opens a session\n"
     "  login           pick a provider, save the API key and pick a model\n"
     "  logout          delete the saved API key of the current provider\n"
     "  report          tell the developers about a problem (shows everything before sending)\n"
     "  telemetry       on | off | --show (beta technical data)\n"
     "  --version       shows the version\n"
     "  --update        checks for, verifies and installs the latest version\n"
     "  --rollback      goes back to the previous version (the copy --update keeps)\n"
     "Provider and model: %s\n",
     "Uso: tinyshell [login | logout | report | telemetry | --version | --update | --rollback | --help]\n"
     "  sin argumentos  abre una sesión\n"
     "  login           elegir proveedor, guardar la API key y elegir modelo\n"
     "  logout          borrar la API key guardada del proveedor actual\n"
     "  report          contar un problema a los desarrolladores (muestra todo antes de enviar)\n"
     "  telemetry       on | off | --show (datos técnicos de la beta)\n"
     "  --version       muestra la versión\n"
     "  --update        busca, verifica e instala la última versión\n"
     "  --rollback      vuelve a la versión anterior (la copia que deja --update)\n"
     "Proveedor y modelo: %s\n"},
    {"You already have the latest version.\n",
     "Ya tenés la última versión.\n"},
    {"[exit=%lu]",
     "[código de salida %lu]"},
    {"[output of %lu bytes left out to save context; run the command again if you need it]",
     "[salida de %lu bytes omitida para ahorrar contexto; volvé a ejecutar el comando si la necesitás]"},
    {"\n"
     "%s answered HTTP %d%s%s%s\n",
     "\n"
     "%s respondió HTTP %d%s%s%s\n"},
    {"\n"
     "%s answered HTTP %lu%s%s%s\n",
     "\n"
     "%s respondió HTTP %lu%s%s%s\n"},
    {"\n"
     "API key for %s (%s): ",
     "\n"
     "API key de %s (%s): "},
    {"\n"
     "Could not reach %s (%s): %s.\n",
     "\n"
     "No se pudo contactar %s (%s): %s.\n"},
    {"\n"
     "Could not reach %s (%s): error %lu, %s.\n",
     "\n"
     "No se pudo contactar %s (%s): error %lu, %s.\n"},
    {"\n"
     "Describe in one line what happened (what you did, what you expected, what came out): ",
     "\n"
     "Contá qué pasó en una línea (qué hiciste, qué esperabas, qué salió): "},
    {"\n"
     "Invalid URL: %s\n",
     "\n"
     "URL inválida: %s\n"},
    {"\n"
     "Invalid provider URL: %s\n",
     "\n"
     "URL del proveedor inválida: %s\n"},
    {"\n"
     "Invalid redirect from %s.\n",
     "\n"
     "Redirección inválida desde %s.\n"},
    {"\n"
     "The response is over %lu bytes; it was discarded.\n",
     "\n"
     "La respuesta supera %lu bytes; se descartó.\n"},
    {"\n"
     "Too many redirects from %s.\n",
     "\n"
     "Demasiadas redirecciones desde %s.\n"},
    {"\n"
     "Warning: WinHTTP refused to enable TLS 1.2 (error %lu). On Windows 7 SP1, install KB3140245.\n",
     "\n"
     "Aviso: WinHTTP no aceptó habilitar TLS 1.2 (error %lu). En Windows 7 SP1 instalá KB3140245.\n"},
    {"\n"
     "[cancelled by the user with Ctrl+C]",
     "\n"
     "[cancelado por el usuario con Ctrl+C]"},
    {"\n"
     "[exit=%lu]",
     "\n"
     "[código de salida %lu]"},
    {"\n"
     "[output truncated: %lu bytes left out]",
     "\n"
     "[salida truncada: %lu bytes omitidos]"},
    {"\n"
     "[timed out: stopped after %d s]",
     "\n"
     "[tiempo agotado: terminado tras %d s]"},
    {"cancelled",
     "cancelado"},
    {"cancelled: no key was entered",
     "cancelado: no se ingresó ninguna clave"},
    {"certificate out of date: check the Windows date and time",
     "certificado fuera de fecha: revisá la fecha y hora de Windows"},
    {"certificate out of date: check this computer's date and time (date)",
     "certificado fuera de fecha: revisá la fecha y hora del equipo (date)"},
    {"chains commands or runs them in the background with &",
     "encadena o deja comandos en segundo plano con &"},
    {"chains several commands with &",
     "encadena varios comandos con &"},
    {"chains several commands with ;",
     "encadena varios comandos con ;"},
    {"chains several commands with ||",
     "encadena varios comandos con ||"},
    {"change it in ",
     "se cambia en "},
    {"changes accounts",
     "modifica cuentas"},
    {"changes disks or partitions",
     "modifica discos o particiones"},
    {"changes drivers",
     "modifica drivers"},
    {"changes file attributes",
     "cambia atributos de archivos"},
    {"changes groups",
     "cambia grupos"},
    {"changes owners",
     "cambia dueños"},
    {"changes partitions",
     "modifica particiones"},
    {"changes passwords",
     "cambia contraseñas"},
    {"changes permissions",
     "cambia permisos"},
    {"changes scheduled tasks",
     "modifica tareas programadas"},
    {"changes services",
     "modifica servicios"},
    {"changes services or accounts",
     "modifica servicios o cuentas"},
    {"changes the boot setup",
     "modifica el arranque"},
    {"changes the file system",
     "modifica el sistema de archivos"},
    {"changes the firewall",
     "modifica el firewall"},
    {"changes the network or the firewall",
     "modifica la red o el firewall"},
    {"changes the registry",
     "modifica el registro"},
    {"changes the runlevel",
     "cambia el nivel de ejecución"},
    {"changes the system through WMI",
     "modifica el sistema vía WMI"},
    {"changes which services start at boot",
     "modifica qué servicios arrancan"},
    {"chkdsk with /f, /r, /x or /b writes to the disk",
     "chkdsk con /f, /r, /x o /b escribe en el disco"},
    {"cmd.exe was not found. Nothing was run.",
     "No se encontró cmd.exe. No se ejecutó nada."},
    {"config.ini: %s (the default is used)",
     "config.ini: %s (se usa el valor por defecto)"},
    {"connect",
     "conectar"},
    {"connection dropped; on Windows 7 this is usually TLS 1.2 not enabled",
     "conexión cortada; en Windows 7 suele ser TLS 1.2 no habilitado"},
    {"converts the file system",
     "convierte el sistema de archivos"},
    {"copies files and may overwrite them",
     "copia archivos y puede sobrescribir"},
    {"copies or syncs files and may delete",
     "copia o sincroniza archivos y puede borrar"},
    {"could not connect (network, proxy or firewall)",
     "no se pudo conectar (red, proxy o firewall)"},
    {"could not save it",
     "no se pudo guardar"},
    {"could not save the key",
     "no se pudo guardar la clave"},
    {"could not save to config.ini (it applies to this session)",
     "no se pudo guardar en config.ini (vale para esta sesión)"},
    {"could not write config.ini",
     "no se pudo escribir config.ini"},
    {"deletes data beyond recovery",
     "borra datos sin posibilidad de recuperarlos"},
    {"deletes files",
     "borra archivos"},
    {"deletes folders",
     "borra carpetas"},
    {"deletes restore points",
     "borra puntos de restauración"},
    {"done \xc2\xb7 %s \xc2\xb7 %s",
     "listo \xc2\xb7 %s \xc2\xb7 %s"},
    {"edits files in place",
     "edita archivos en el lugar"},
    {"empties files",
     "vacía archivos"},
    {"encrypts or wipes disk data",
     "cifra o borra datos del disco"},
    {"ends processes",
     "termina procesos"},
    {"find with -delete or -exec changes or runs things",
     "find con -delete o -exec modifica o ejecuta"},
    {"formats a swap partition",
     "formatea una partición de swap"},
    {"formats disks",
     "formatea discos"},
    {"installs or removes packages",
     "instala o desinstala paquetes"},
    {"installs or uninstalls software",
     "instala o desinstala software"},
    {"installs or uninstalls updates",
     "instala o desinstala actualizaciones"},
    {"keeping the default model (/model to change it)",
     "se mantiene el modelo por defecto (/model para cambiarlo)"},
    {"key for %s saved in %s",
     "clave de %s guardada en %s"},
    {"key received (%d characters)",
     "clave recibida (%d caracteres)"},
    {"loads drivers",
     "carga drivers"},
    {"loads or removes drivers",
     "carga o quita drivers"},
    {"model: ",
     "modelo: "},
    {"model: empty or with spaces",
     "model: vacío o con espacios"},
    {"mounts file systems",
     "monta sistemas de archivos"},
    {"moves files",
     "mueve archivos"},
    {"moves or renames files and may overwrite",
     "mueve o renombra archivos y puede sobrescribir"},
    {"name: use letters, digits, '-', '_' or '.'",
     "name: usá letras, números, '-', '_' o '.'"},
    {"network error",
     "error de red"},
    {"new conversation",
     "conversación nueva"},
    {"no API key",
     "sin API key"},
    {"nothing was sent",
     "no se envió nada"},
    {"nothing was sent; the report is saved in the file above",
     "no se envió nada; el informe quedó guardado en el archivo de arriba"},
    {"number or name [%d]: ",
     "número o nombre [%d]: "},
    {"off",
     "desactivada"},
    {"on",
     "activada"},
    {"open a session",
     "abrir sesión"},
    {"provider: ",
     "proveedor: "},
    {"receive",
     "recibir"},
    {"redirects with > (creates or overwrites files)",
     "redirige con > (crea o sobrescribe archivos)"},
    {"registers or runs code from a DLL",
     "registra o ejecuta código de una DLL"},
    {"removes drivers",
     "quita drivers"},
    {"renames files",
     "renombra archivos"},
    {"repairs system files",
     "repara archivos del sistema"},
    {"report #%s sent%s%s \xc2\xb7 thanks",
     "reporte #%s enviado%s%s \xc2\xb7 gracias"},
    {"report sent",
     "reporte enviado"},
    {"restarts the computer",
     "reinicia el equipo"},
    {"right-click to paste",
     "clic derecho para pegar"},
    {"runs PowerShell",
     "ejecuta PowerShell"},
    {"runs a script",
     "ejecuta un script"},
    {"runs as root",
     "ejecuta como root"},
    {"runs code from a DLL",
     "ejecuta código de una DLL"},
    {"saved \xc2\xb7 tinyshell logout deletes it",
     "guardada \xc2\xb7 tinyshell logout la borra"},
    {"send",
     "enviar"},
    {"session_header: only letters, digits and '-'",
     "session_header: solo letras, números y '-'"},
    {"shuts down or restarts the computer",
     "apaga o reinicia el equipo"},
    {"shuts down the computer",
     "apaga el equipo"},
    {"switches user",
     "cambia de usuario"},
    {"the API key was rejected; nothing was saved",
     "la API key fue rechazada; no se guardó nada"},
    {"the Windows Credential Manager",
     "el Administrador de credenciales de Windows"},
    {"the certificate does not match the host (an intercepting proxy?)",
     "el certificado no coincide con el host (¿un proxy que intercepta?)"},
    {"the certificate does not match the server (an intercepting proxy?)",
     "el certificado no coincide con el servidor (¿un proxy que intercepta?)"},
    {"the connection dropped",
     "la conexión se cortó"},
    {"the model list came back empty or in an unknown format",
     "la lista de modelos vino vacía o en un formato desconocido"},
    {"the report server",
     "el servidor de reportes"},
    {"the server",
     "el servidor"},
    {"the server (http)",
     "el servidor (http)"},
    {"the server name did not resolve (DNS or no network)",
     "no se resolvió el servidor (DNS o sin red)"},
    {"thinking...",
     "pensando..."},
    {"this provider does not publish its model list; use /model <name>",
     "este proveedor no publica su lista de modelos; usá /model <nombre>"},
    {"timed out",
     "tiempo de espera agotado"},
    {"turns swap off",
     "desactiva la swap"},
    {"unknown key",
     "clave desconocida"},
    {"unmounts file systems",
     "desmonta sistemas de archivos"},
    {"untrusted certificate (a proxy intercepting HTTPS?)",
     "certificado no confiable (¿un proxy que intercepta HTTPS?)"},
    {"untrusted certificate: update the root certificates",
     "certificado no confiable: actualizá los certificados raíz"},
    {"url: has spaces or invalid characters",
     "url: tiene espacios o caracteres inválidos"},
    {"url: must start with https:// or http://",
     "url: debe empezar con https:// o http://"},
    {"uses $(...), which can hide the real command",
     "usa $(...), que puede ocultar el comando real"},
    {"uses ^, which can hide the real command",
     "usa ^, que puede ocultar el comando real"},
    {"uses `...`, which can hide the real command",
     "usa `...`, que puede ocultar el comando real"},
    {"using the saved API key \xc2\xb7 /login to change it",
     "usando la API key guardada \xc2\xb7 /login para cambiarla"},
    {"warning: the URL is http://, the API key and the conversation travel unencrypted",
     "aviso: la URL es http://, la API key y la conversación viajan sin cifrar"},
    {"wipes disk signatures",
     "borra firmas de discos"},
    {"writes files",
     "escribe archivos"},
    {"writes raw blocks (can overwrite a disk)",
     "escribe bloques crudos (puede pisar un disco)"},
    {"~/.config/tinyshell/credentials (only your user can read it)",
     "~/.config/tinyshell/credentials (solo tu usuario puede leerlo)"},
};

const int I18N_ES_COUNT = (int)(sizeof I18N_ES / sizeof I18N_ES[0]);
