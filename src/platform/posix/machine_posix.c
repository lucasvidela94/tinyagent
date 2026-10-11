#include "posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "machine.h"
#include "sys.h"

/* The value of `key` in a file of "KEY=value" or "KEY: value" lines, quotes removed. */
static int file_value(const char *path, const char *key, char *out, size_t cap) {
  size_t kn = strlen(key);
  char *text = sys_read_file(path, 64 * 1024, NULL), *p = text;
  int ok = 0;
  while (p && *p && !ok) {
    size_t n = strcspn(p, "\n");
    if (n > kn && !strncmp(p, key, kn) && (p[kn] == '=' || p[kn] == ':')) {
      const char *v = p + kn + 1;
      size_t vn = n - kn - 1;
      while (vn && (*v == ' ' || *v == '\t' || *v == '"' || *v == '\'')) v++, vn--;
      while (vn && (v[vn - 1] == '"' || v[vn - 1] == '\'' || v[vn - 1] == ' ' || v[vn - 1] == '\r')) vn--;
      if (vn && vn < cap) {
        memcpy(out, v, vn);
        out[vn] = 0;
        ok = 1;
      }
    }
    p += n + (p[n] == '\n');
  }
  free(text);
  return ok;
}

/* /etc/issue's first line without getty escapes ("Debian GNU/Linux 6.0 \n \l"). */
static int issue_name(char *out, size_t cap) {
  char *text = sys_read_file("/etc/issue", 4096, NULL), *bs;
  size_t n;
  int ok = 0;
  if (text) {
    n = strcspn(text, "\n");
    text[n] = 0;
    if ((bs = strchr(text, '\\')) != NULL) *bs = 0;
    n = strlen(text);
    while (n && text[n - 1] == ' ') text[--n] = 0;
    if (n && n < cap) {
      memcpy(out, text, n + 1);
      ok = 1;
    }
  }
  free(text);
  return ok;
}

void system_os(Buf *b) {
  char name[256];
  struct utsname u;
  if (file_value("/etc/os-release", "PRETTY_NAME", name, sizeof name) ||
      file_value("/etc/lsb-release", "DISTRIB_DESCRIPTION", name, sizeof name) || issue_name(name, sizeof name))
    buf_str(b, name);
  else
    buf_str(b, "Linux");
  if (uname(&u) == 0) {
    buf_str(b, " (kernel ");
    buf_str(b, u.release);
    buf_str(b, ")");
  }
}

static unsigned long meminfo_mb(const char *key) { /* "MemTotal:  2048000 kB" -> MB, 0 if absent */
  char v[64];
  return file_value("/proc/meminfo", key, v, sizeof v) ? strtoul(v, NULL, 10) / 1024 : 0;
}

void system_details(Buf *b) {
  struct utsname u;
  char num[160];
  unsigned long total = meminfo_mb("MemTotal"), avail = meminfo_mb("MemAvailable");
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);
  const char *bits = "unknown architecture";
  if (!avail) avail = meminfo_mb("MemFree") + meminfo_mb("Buffers") + meminfo_mb("Cached"); /* kernels before 3.14 */
  if (uname(&u) == 0)
    bits = strstr(u.machine, "64") ? "64-bit" : (u.machine[0] == 'i' && strstr(u.machine, "86")) ? "32-bit" : u.machine;
  system_os(b);
  snprintf(num, sizeof num, ", %s, %lu MB RAM (%lu MB free), %ld logical CPUs, current directory ", bits, total,
           avail, cpus > 0 ? cpus : 1);
  buf_str(b, num);
  sys_cwd(b);
  buf_str(b, ".");
}

const Dialect *system_dialect(void) { return &DIALECT_SH; }
