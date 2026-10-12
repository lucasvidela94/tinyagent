#include "posix.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "sys.h"

const char SYS_EOL[] = "\n";
const char SYS_HOME_LABEL[] = "~";
const char SYS_RELEASE_ASSET[] = "tinyshell-linux-i386";

static unsigned long last_error;

static int kept(int ok) { /* records errno before free() can touch it */
  if (!ok) last_error = (unsigned long)errno;
  return ok;
}

void sys_random(unsigned char *out, size_t n) {
  size_t got = 0, i;
  int fd = open("/dev/urandom", O_RDONLY);
  while (fd >= 0 && got < n) {
    ssize_t r = read(fd, out + got, n - got);
    if (r <= 0) break;
    got += (size_t)r;
  }
  if (fd >= 0) close(fd);
  for (i = got; i < n; i++) out[i] = (unsigned char)((unsigned long)time(NULL) >> (i % 4 * 8) ^ (unsigned long)getpid() ^ (i * 37));
}

unsigned long sys_ms(void) {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t)) return 0;
  return (unsigned long)t.tv_sec * 1000ul + (unsigned long)(t.tv_nsec / 1000000);
}

void sys_cwd(Buf *out) {
  char p[4096];
  if (getcwd(p, sizeof p)) buf_str(out, p);
}

void sys_identity(Buf *host, Buf *user, Buf *home) {
  char h[256];
  const char *u = getenv("USER"), *d = getenv("HOME");
  if (!u || !*u) u = getenv("LOGNAME");
  if (!gethostname(h, sizeof h)) {
    h[sizeof h - 1] = 0;
    buf_str(host, h);
  }
  if (u) buf_str(user, u);
  if (d) buf_str(home, d);
}

/* $XDG_CONFIG_HOME/tinyshell, else ~/.config/tinyshell; private to the user (0700): it
 * holds reports with whole conversations. */
static int data_dir(Buf *dir) {
  const char *xdg = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME");
  if (xdg && *xdg == '/') buf_str(dir, xdg);
  else if (home && *home) {
    buf_str(dir, home);
    buf_str(dir, "/.config");
    mkdir(dir->p, 0700);
  } else return 0;
  buf_str(dir, "/tinyshell");
  return mkdir(dir->p, 0700) == 0 || errno == EEXIST;
}

int sys_data_path(const char *name, Buf *out) {
  Buf dir = {0};
  const char *p;
  int ok = data_dir(&dir);
  for (p = name; ok && (p = strchr(p, '/')) != NULL; p++) { /* folders inside the data dir */
    size_t len = dir.len;
    buf_str(&dir, "/");
    buf_add(&dir, name, (size_t)(p - name));
    ok = mkdir(dir.p, 0700) == 0 || errno == EEXIST;
    dir.len = len;
    dir.p[len] = 0;
  }
  if (ok) {
    buf_add(out, dir.p, dir.len);
    buf_str(out, "/");
    buf_str(out, name);
  }
  free(dir.p);
  return ok;
}

/* Files that report no size (/proc, pipes): read until EOF, keeping at most max bytes. */
static char *read_unsized(int fd, size_t max, size_t *len) {
  Buf b = {0};
  size_t dropped = 0;
  char chunk[4096];
  ssize_t r;
  while ((r = read(fd, chunk, sizeof chunk)) > 0) buf_add_capped(&b, chunk, (size_t)r, max, &dropped);
  if (len) *len = b.len;
  return buf_take(&b);
}

char *sys_read_file(const char *path, size_t max, size_t *len) {
  int fd = open(path, O_RDONLY);
  struct stat st;
  size_t size, got = 0;
  char *data;
  if (fd < 0) return NULL;
  if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size == 0) {
    data = read_unsized(fd, max, len);
    close(fd);
    return data;
  }
  size = fstat(fd, &st) == 0 && st.st_size > 0 ? (size_t)st.st_size : 0;
  if (size > max) {
    lseek(fd, -(off_t)max, SEEK_END); /* keep the end */
    size = max;
  }
  data = (char *)xrealloc(NULL, size + 1);
  while (got < size) {
    ssize_t r = read(fd, data + got, size - got);
    if (r <= 0) break;
    got += (size_t)r;
  }
  close(fd);
  data[got] = 0;
  if (len) *len = got;
  return data;
}

int sys_write_file(const char *path, const char *data, size_t n, int flags) {
  int fd = open(path, O_WRONLY | O_CREAT | ((flags & SYS_APPEND) ? O_APPEND : O_TRUNC),
                (flags & SYS_EXECUTABLE) ? 0700 : 0600);
  size_t done = 0;
  if (!kept(fd >= 0)) return 0;
  if (flags & SYS_EXECUTABLE) fchmod(fd, 0700); /* an existing file keeps its mode otherwise */
  while (done < n) {
    ssize_t w = write(fd, data + done, n - done);
    if (w <= 0) break;
    done += (size_t)w;
  }
  return kept(close(fd) == 0 && done == n);
}

int sys_self_path(Buf *out) {
  char p[4096];
  ssize_t n = readlink("/proc/self/exe", p, sizeof p - 1);
  if (!kept(n > 0)) return 0;
  p[n] = 0;
  buf_str(out, p);
  return 1;
}

int sys_rename(const char *from, const char *to) { return kept(rename(from, to) == 0); }
int sys_remove(const char *path) { return kept(unlink(path) == 0); }

int sys_exists(const char *path) {
  struct stat st;
  return kept(stat(path, &st) == 0);
}

unsigned long sys_error(void) { return last_error; }
