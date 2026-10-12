/* Subcommands and flags (`tinyshell login`, `--update`...). Canonical names are English;
 * the earlier Spanish names stay as aliases so existing habits and scripts keep working. */
#ifndef TINYSHELL_CLI_H
#define TINYSHELL_CLI_H

/* Runs argv[1] (with argv[2] as its argument, if any). Returns the process exit code. */
int cli_run(int argc, char **argv);

#endif
