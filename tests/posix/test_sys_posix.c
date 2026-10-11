/* Tests for src/platform/posix/sys_posix.c, run natively on Linux by build/test-core.sh.
 * Everything happens under a fresh temporary $XDG_CONFIG_HOME. */
#include "posix.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../check.h"
#include "sys.h"

static char root[] = "/tmp/tinyshell-test-XXXXXX";

static mode_t mode_of(const char *path) {
  struct stat st;
  return stat(path, &st) ? 0 : (st.st_mode & 0777);
}

static void test_data_path(void) {
  Buf p = {0}, dir = {0};
  CHECK(sys_data_path("reportes/r.txt", &p));
  CHECK(p.p && !strcmp(p.p + strlen(root), "/tinyshell/reportes/r.txt"));
  buf_str(&dir, root);
  buf_str(&dir, "/tinyshell");
  CHECK(mode_of(dir.p) == 0700); /* reports hold whole conversations */
  buf_str(&dir, "/reportes");
  CHECK(mode_of(dir.p) == 0700);
  free(p.p);
  free(dir.p);
}

static void test_files(void) {
  Buf p = {0};
  size_t n = 0;
  char *t;
  sys_data_path("log.txt", &p);
  CHECK(sys_write_file(p.p, "uno\n", 4, 0));
  CHECK(sys_write_file(p.p, "dos\n", 4, SYS_APPEND));
  CHECK(mode_of(p.p) == 0600);
  t = sys_read_file(p.p, 100, &n);
  CHECK(t && n == 8 && !strcmp(t, "uno\ndos\n"));
  free(t);
  t = sys_read_file(p.p, 4, &n); /* only the end */
  CHECK(t && n == 4 && !strcmp(t, "dos\n"));
  free(t);
  CHECK(sys_write_file(p.p, "x", 1, 0)); /* replaces, does not append */
  t = sys_read_file(p.p, 100, &n);
  CHECK(t && !strcmp(t, "x"));
  free(t);
  CHECK(sys_read_file("/no/such/file", 10, NULL) == NULL);
  free(p.p);
}

static void test_update_primitives(void) {
  Buf a = {0}, b = {0}, self = {0};
  size_t n;
  char *t;
  sys_data_path("bin.new", &a);
  sys_data_path("bin", &b);
  CHECK(sys_write_file(a.p, "#!/bin/sh\n", 10, SYS_EXECUTABLE));
  CHECK(mode_of(a.p) == 0700);
  CHECK(sys_write_file(b.p, "old", 3, 0));
  CHECK(sys_rename(a.p, b.p)); /* replaces the target */
  CHECK(!sys_exists(a.p) && sys_exists(b.p));
  t = sys_read_file(b.p, 100, &n);
  CHECK(t && !strcmp(t, "#!/bin/sh\n"));
  free(t);
  CHECK(!sys_rename(a.p, b.p) && sys_error() == ENOENT);
  CHECK(sys_remove(b.p) && !sys_exists(b.p));
  CHECK(sys_self_path(&self) && sys_exists(self.p) && strstr(self.p, "test_sys_posix"));
  free(a.p);
  free(b.p);
  free(self.p);
}

static void test_clock_and_random(void) {
  unsigned char r1[16], r2[16];
  unsigned long t0 = sys_ms(), t1;
  struct timespec wait = {0, 20000000};
  nanosleep(&wait, NULL);
  t1 = sys_ms();
  CHECK(t1 - t0 >= 15 && t1 - t0 < 2000);
  sys_random(r1, sizeof r1);
  sys_random(r2, sizeof r2);
  CHECK(memcmp(r1, r2, sizeof r1) != 0);
}

int main(void) {
  int r;
  if (!mkdtemp(root)) return 1;
  setenv("XDG_CONFIG_HOME", root, 1);
  test_data_path();
  test_files();
  test_update_primitives();
  test_clock_and_random();
  r = CHECK_DONE("test_sys_posix");
  { /* remove what the tests left: log.txt and the two folders */
    const char *left[] = {"/tinyshell/log.txt", "/tinyshell/reportes", "/tinyshell", ""};
    int i;
    for (i = 0; i < 4; i++) {
      Buf p = {0};
      buf_str(&p, root);
      buf_str(&p, left[i]);
      if (unlink(p.p) && rmdir(p.p)) r = 1;
      free(p.p);
    }
  }
  return r;
}
