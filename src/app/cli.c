#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "config.h"
#include "console.h"
#include "credential.h"
#include "i18n.h"
#include "login.h"
#include "provider.h"
#include "reporter.h"
#include "settings.h"
#include "telemetry.h"
#include "update.h"

static int usage(void) {
  say(tr("Usage: tinyshell [login | logout | report | telemetry | --version | --update | --rollback | --help]\n"
         "  (no arguments)  opens a session\n"
         "  login           pick a provider, save the API key and pick a model\n"
         "  logout          delete the saved API key of the current provider\n"
         "  report          tell the developers about a problem (shows everything before sending)\n"
         "  telemetry       on | off | --show (beta technical data)\n"
         "  --version       shows the version\n"
         "  --update        checks for, verifies and installs the latest version\n"
         "  --rollback      goes back to the previous version (the copy --update keeps)\n"
         "Provider and model: %s\n"),
      settings_path());
  return 0;
}

static int is(const char *s, const char *a, const char *b, const char *c) {
  return !strcmp(s, a) || (b && !strcmp(s, b)) || (c && !strcmp(s, c));
}

int cli_run(int argc, char **argv) {
  const char *flag = argv[1], *arg = argc > 2 ? argv[2] : NULL;
  Provider p;
  settings_load(&p);
  if (is(flag, "--version", "-v", NULL)) {
    say("tinyshell %s\n", TINYSHELL_VERSION);
    return 0;
  }
  if (is(flag, "--update", "update", NULL)) return update_run();
  if (is(flag, "--rollback", "rollback", NULL)) return update_rollback();
  if (is(flag, "login", "--login", NULL)) return login_run(NULL, NULL, 0) ? 0 : 1;
  if (is(flag, "report", "reporte", NULL)) {
    telemetry_init();
    return reporter_run(NULL);
  }
  if (is(flag, "telemetry", "telemetria", NULL)) return telemetry_command(arg);
  if (is(flag, "logout", "--logout", NULL)) {
    say(credential_delete(p.name) ? tr("API key for %s deleted.\n") : tr("Could not delete the API key for %s.\n"),
        p.name);
    return 0;
  }
  if (is(flag, "--help", "-h", "/?")) return usage();
  say(tr("Unknown option: %s\n"), flag);
  usage();
  return 2;
}
