/* Tests for src/platform/posix/machine_posix.c on the Linux host running them. */
#include "posix.h"

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "../check.h"
#include "machine.h"

int main(void) {
  Buf os = {0}, d = {0};
  struct utsname u;
  system_os(&os);
  system_details(&d);
  uname(&u);
  CHECK(os.p && strstr(os.p, u.release) && strstr(os.p, " (kernel ")); /* distro name + running kernel */
  CHECK(os.p && strncmp(os.p, " (kernel", 8) != 0);                   /* some name came first */
  CHECK(d.p && !strncmp(d.p, os.p, os.len));                           /* details start with the OS */
  CHECK(d.p && strstr(d.p, "-bit, ") && strstr(d.p, " MB RAM (") && strstr(d.p, " logical CPUs, current directory /"));
  CHECK(d.p && !strstr(d.p, ", 0 MB RAM")); /* /proc/meminfo was read */
  CHECK(system_dialect() == &DIALECT_SH);
  free(os.p);
  free(d.p);
  return CHECK_DONE("test_machine_posix");
}
