/* Beta telemetry (Win32 adapter). On by default while the version is 0.x, announced on
 * first run, off with `tinyshell telemetry off`. Metadata only: never prompts, commands
 * or their output. Each event is one short request (a few seconds at most, silent on
 * failure) made in the moment; nothing runs in the background. Every event sent is also
 * appended to telemetry.log in the data folder so the user can see it. */
#ifndef TINYSHELL_TELEMETRY_H
#define TINYSHELL_TELEMETRY_H

void telemetry_init(void);           /* loads the setting; prints the notice once */
int telemetry_enabled(void);
const char *telemetry_install_id(void); /* random, created on first run */

/* name: session_start, session_end or error; data_json: a small JSON object or NULL. */
void telemetry_event(const char *name, const char *data_json);

/* Records an error for /reporte and, if enabled, sends an "error" event. */
void telemetry_error(const char *kind, const char *code, const char *stage);

int telemetry_command(const char *arg); /* `tinyshell telemetry on|off|--show` */

#endif
