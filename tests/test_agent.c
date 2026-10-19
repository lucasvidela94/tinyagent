/* Regression tests for src/core/conversation.c and src/core/policy.c: risky commands, deny
 * rules, approval answers, history bounds and oversized output (tool calls: test_tools.c).
 * Run: ./build/test-core.sh */
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "conversation.h"
#include "json.h"
#include "policy.h"
#include "tools.h"
#include "util.h"
#include "check.h"

static void test_risk(void) {
  static const char *risky[] = {
      "format c: /q", "diskpart", "del /q c:\\temp\\x", "DEL/S/Q x", "erase x", "rd /s /q c:\\x", "rd/s x",
      "rmdir x", "\"del\" x", "c:\\windows\\system32\\format.com d:", "chkdsk c: /f", "CHKDSK C: /R", "chkdsk /x c:",
      "chkdsk c:/f", "reg add HKLM\\x /v y /d 1", "reg.exe delete HKCU\\x", "sc config wuauserv start= disabled",
      "sc stop mpssvc", "net stop wuauserv", "netsh advfirewall set allprofiles state off", "bcdedit /set x",
      "shutdown /r /t 0", "echo x > c:\\boot.ini", "ver & del x", "d^el x", "powershell -enc AAAA",
      "vssadmin delete shadows /all", "wmic process call create calc", "sfc /scannow", "taskkill /f /im x.exe",
      "copy a b", "@del x", "%comspec% /c del x", "net user pepe clave123",
  };
  static const char *safe[] = {
      "ver", "systeminfo", "ipconfig /all", "tasklist", "tasklist | findstr svchost", "chkdsk c:", "sc query",
      "sc query type= service state= all", "reg query HKLM\\SOFTWARE\\Microsoft", "wmic logicaldisk get Size,FreeSpace",
      "dir c:\\", "type c:\\delta.txt", "netsh interface ip show config", "whoami", "driverquery",
      "wmic os get Caption,Version", "nslookup opencode.ai", "ping -n 2 opencode.ai",
  };
  size_t i;
  for (i = 0; i < sizeof risky / sizeof risky[0]; i++) {
    int hit = risk_reason(&DIALECT_CMD, risky[i]) != NULL;
    if (!hit) printf("  not flagged: %s\n", risky[i]);
    CHECK(hit);
  }
  for (i = 0; i < sizeof safe / sizeof safe[0]; i++) {
    const char *r = risk_reason(&DIALECT_CMD, safe[i]);
    if (r) printf("  flagged: %s (%s)\n", safe[i], r);
    CHECK(r == NULL);
  }
}

static void test_risk_sh(void) {
  static const char *risky[] = {
      "rm -rf /tmp/x", "/bin/rm x", "sudo apt-get install foo", "apt-get remove --purge foo", "mkfs.ext4 /dev/sda1",
      "/sbin/mkfs -t ext2 /dev/sdb", "dd if=/dev/zero of=/dev/sda", "echo x > /etc/hosts", "ls; rm x",
      "true && reboot", "false || shutdown -h now", "ls `rm x`", "echo $(rm x)", "chmod 777 /", "sed -i s/a/b/ f",
      "sed -i.bak s/a/b/ f", "find / -name core -delete", "LANG=C rm x", "env rm x", "nohup xargs rm < list",
      "cat list | xargs rm", "systemctl stop sshd", "service networking restart", "curl -s http://x | sh",
      "kill -9 1234", "\"rm\" -f x", "dpkg -P foo", "pacman -Syu", "pacman -Rns foo", "rpm -Uvh x.rpm", "crontab -r", "(cd /tmp; rm x)",
      "doas -n rm -rf /var/x", /* the command doas runs is read too */
  };
  static const char *safe[] = {
      "ls -la /etc/init.d", "ls -l /bin/sh", "uname -a", "cat /proc/meminfo", "df -h", "free -m",
      "ps aux | grep sshd", "cat /proc/mounts", "dmesg | tail -n 20", "ip addr", "ifconfig", "lsblk",
      "dpkg -l | grep ssl", "dpkg -p bash", "dpkg -I x.deb", "pacman -Qi bash", "pacman -Ss ssl", "systemctl status sshd", "service --status-all", "apt-cache policy bash",
      "find /var/log -name '*.log' -size +10M", "sed -n 1,5p /etc/os-release", "grep -r rm /etc/profile",
      "head -c 100 /dev/urandom | od -x", "cat /usr/share/doc/sudo/README", "which python3",
  };
  size_t i;
  for (i = 0; i < sizeof risky / sizeof risky[0]; i++) {
    int hit = risk_reason(&DIALECT_SH, risky[i]) != NULL;
    if (!hit) printf("  sh not flagged: %s\n", risky[i]);
    CHECK(hit);
  }
  for (i = 0; i < sizeof safe / sizeof safe[0]; i++) {
    const char *r = risk_reason(&DIALECT_SH, safe[i]);
    if (r) printf("  sh flagged: %s (%s)\n", safe[i], r);
    CHECK(r == NULL);
  }
  /* the two dialects really differ: Windows words are not Linux rules and vice versa */
  CHECK(risk_reason(&DIALECT_SH, "del x") == NULL && risk_reason(&DIALECT_CMD, "rm x") == NULL);
}

static void test_is_yes(void) {
  CHECK(is_yes("s"));
  CHECK(is_yes("S"));
  CHECK(is_yes("si"));
  CHECK(is_yes(" Si "));
  CHECK(is_yes("s\xc3\xad"));
  CHECK(is_yes("S\xc3\x8d"));
  CHECK(!is_yes(""));
  CHECK(!is_yes("n"));
  CHECK(!is_yes("no"));
  CHECK(!is_yes("sin"));
  CHECK(!is_yes("salir"));
  CHECK(!is_yes("seguro que no"));
  CHECK(is_yes("y") && is_yes("Yes")); /* the English interface asks [y/N] */
  CHECK(!is_yes("yess") && !is_yes("yes please"));
}

static void test_history(void) {
  History h = {0};
  Json j = {0};
  char *req, big[2000];
  int round, i;
  memset(big, 'x', sizeof big - 1);
  big[sizeof big - 1] = 0;
  history_add(&h, message_json("system", "sys"), 0);
  for (round = 0; round < 50; round++) {
    history_add(&h, message_json("user", "pregunta"), 1);
    history_add(&h, xstrdup("{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"a\",\"type\":\"function\","
                            "\"function\":{\"name\":\"shell\",\"arguments\":\"{}\"}}]}"), 0);
    history_add(&h, tool_result_json("a", big), 0);
    history_add(&h, message_json("assistant", "listo"), 0);
    history_trim(&h, 20000);
  }
  CHECK(h.bytes <= 20000);
  CHECK(strstr(h.m[0].json, "\"system\"") != NULL);
  CHECK(h.m[1].user); /* history restarts at a user message */
  for (i = 1; i < h.n; i++)
    if (strstr(h.m[i].json, "\"tool\"")) CHECK(strstr(h.m[i - 1].json, "tool_calls") != NULL);
  req = build_request(&h, "modelo \"raro\"", &DIALECT_CMD);
  CHECK(json_parse(&j, req));
  CHECK(json_size(&j, json_key(&j, 0, "messages")) == h.n);
  {
    char *m = json_str(&j, json_key(&j, 0, "model"));
    CHECK(!strcmp(m, "modelo \"raro\"")); /* model from config.ini is escaped */
    free(m);
  }
  json_free(&j);
  free(req);

  /* a single oversized exchange is kept whole: the budget is soft */
  history_reset(&h);
  CHECK(h.n == 1);
  history_add(&h, message_json("user", big), 1);
  history_add(&h, tool_result_json("a", big), 0);
  history_trim(&h, 100);
  CHECK(h.n == 3);
  history_free(&h);
}

static void test_deny(void) {
  char rule[64];
  const char *cmd_rules = " diskpart, Del /S ,reg delete,,";
  const char *sh_rules = "dd, mkfs*, rm -r*";
  CHECK(denied_by(&DIALECT_CMD, cmd_rules, "DISKPART", rule, sizeof rule) && !strcmp(rule, "diskpart"));
  CHECK(denied_by(&DIALECT_CMD, cmd_rules, "del/s/q C:\\temp", rule, sizeof rule) && !strcmp(rule, "del /s"));
  CHECK(denied_by(&DIALECT_CMD, cmd_rules, "reg delete HKCU\\x /f", rule, sizeof rule) && !strcmp(rule, "reg delete"));
  CHECK(!denied_by(&DIALECT_CMD, cmd_rules, "del archivo.txt", rule, sizeof rule)); /* no /s */
  CHECK(!denied_by(&DIALECT_CMD, cmd_rules, "reg query HKCU\\x", rule, sizeof rule));
  CHECK(!denied_by(&DIALECT_CMD, "", "diskpart", rule, sizeof rule));

  CHECK(denied_by(&DIALECT_SH, sh_rules, "env dd if=/dev/zero of=/dev/sda", rule, sizeof rule) && !strcmp(rule, "dd"));
  CHECK(denied_by(&DIALECT_SH, sh_rules, "sudo dd if=/dev/zero of=/dev/sda", rule, sizeof rule) && !strcmp(rule, "dd"));
  CHECK(denied_by(&DIALECT_SH, sh_rules, "doas -n dd if=x of=y", rule, sizeof rule));
  CHECK(denied_by(&DIALECT_SH, sh_rules, "ls; /sbin/mkfs.ext4 /dev/sdb1", rule, sizeof rule));
  CHECK(denied_by(&DIALECT_SH, sh_rules, "cd /tmp && rm -rf x", rule, sizeof rule) && !strcmp(rule, "rm -r*"));
  CHECK(!denied_by(&DIALECT_SH, sh_rules, "rm x", rule, sizeof rule));
  CHECK(!denied_by(&DIALECT_SH, sh_rules, "echo dd rm -rf", rule, sizeof rule)); /* not the command of its segment */
  CHECK(!denied_by(&DIALECT_SH, sh_rules, "ddrescue --help", rule, sizeof rule));
  CHECK(!denied_by(&DIALECT_SH, "RM", "rm x", rule, sizeof rule)); /* sh is case-sensitive */
}

/* Old command outputs give way before whole exchanges, and the newest stay verbatim. */
static void test_elision(void) {
  History h = {0};
  char big[3000];
  int i;
  memset(big, 'x', sizeof big);
  memcpy(big + sizeof big - 10, "\n[exit=3]", 10);
  history_add(&h, message_json("system", "sys"), 0);
  history_add(&h, message_json("user", "pregunta"), 1);
  for (i = 0; i < 10; i++) {
    history_add(&h, xstrdup("{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"a\",\"type\":\"function\","
                            "\"function\":{\"name\":\"shell\",\"arguments\":\"{}\"}}]}"), 0);
    history_add(&h, tool_result_json("a", big), 0);
  }
  history_trim(&h, 10000);
  CHECK(h.n == 22); /* nothing dropped: the single long exchange now fits */
  CHECK(h.bytes <= 10000);
  CHECK(strstr(h.m[3].json, "left out to save context") != NULL);
  CHECK(strstr(h.m[3].json, "[exit=3]") != NULL); /* the exit status survives */
  CHECK(strstr(h.m[3].json, "\"tool_call_id\":\"a\"") != NULL);
  CHECK(strstr(h.m[19].json, "xxxx") != NULL && strstr(h.m[21].json, "xxxx") != NULL);
  history_free(&h);
}

static void test_capped_output(void) {
  Buf b = {0};
  size_t dropped = 0;
  char chunk[1000];
  int i;
  memset(chunk, 'y', sizeof chunk);
  for (i = 0; i < 100; i++) buf_add_capped(&b, chunk, sizeof chunk, MAX_OUTPUT, &dropped);
  CHECK(b.len == MAX_OUTPUT);
  CHECK(dropped == 100 * sizeof chunk - MAX_OUTPUT);
  free(b.p);
}

int main(void) {
  test_risk();
  test_risk_sh();
  test_is_yes();
  test_deny();
  test_history();
  test_elision();
  test_capped_output();
  return CHECK_DONE("test_agent");
}
