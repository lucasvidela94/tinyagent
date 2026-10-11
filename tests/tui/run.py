"""Drives tests/tui/demo in a pseudo-terminal and checks the screen (pyte emulates the
terminal, including the cursor-position reply the TUI asks for).
Usage: python3 tests/tui/run.py path/to/demo   (needs the pyte package)."""
import os, pty, select, sys, time
import pyte

def run(demo, lang, keys):
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.update({"TERM": "xterm", "LANG": lang, "LC_ALL": "", "LC_CTYPE": ""})
        os.execv(demo, [demo])
    screen = pyte.Screen(80, 24)
    stream = pyte.ByteStream(screen)
    screen.write_process_input = lambda data: os.write(fd, data.encode())
    def pump(seconds):
        end = time.time() + seconds
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try:
                    data = os.read(fd, 4096)
                except OSError:
                    return False
                if not data:
                    return False
                stream.feed(data)
        return True
    import fcntl, struct, termios
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    pump(0.5)
    at_prompt = [line.rstrip() for line in screen.display]  # before the first key
    for k in keys:
        os.write(fd, k)
        pump(0.3)
    pump(0.5)
    os.waitpid(pid, 0)
    return [line.rstrip() for line in screen.display], screen, at_prompt

failures = 0
def check(cond, what, lines):
    global failures
    if not cond:
        failures += 1
        print("FAIL:", what)
        print("\n".join(lines))

demo = sys.argv[1]
keys = [b"hola agente\r", b"s\r", b"\x1b[B", b"\x1b[B", b"\r", "sk-ñ12".encode(), b"\x7f", b"3\r", b"\x04"]
lines, screen, prompt = run(demo, "es_AR.UTF-8", keys)
text = "\n".join(lines)
check("hola agente" in text, "the user block shows the message", lines)
user_row = next((i for i, l in enumerate(lines) if "hola agente" in l), -1)
check(user_row >= 0 and screen.buffer[user_row][2].reverse, "the user block is shaded", lines)
check("Hola, ¿qué tal? Miro los archivos." in text, "streamed text, UTF-8 kept", lines)
check("pensando" not in text, "the thinking line is erased", lines)
check("$ ls -la" in text and "uno.txt" in text, "proposal, approval and output", lines)
check("glm-5" in text and "kimi" not in text, "menu: two arrows down, Enter; list erased", lines)
# "sk-ñ13": 6 characters (6 stars), 7 bytes in UTF-8 (the demo prints the byte length)
check("API key: ******\n" in text + "\n" and "clave de 7 caracteres" in text, "secret: one * per character, Backspace", lines)
check("EOF" in text, "Ctrl+D at the prompt is EOF", lines)
ptext = "\n".join(prompt)
check("─" * 20 in ptext and ">" in ptext and "deepseek-v4.1-flash" in ptext, "editor between rules, footer", prompt)
used = [l for l in prompt if l]
check(used[-1].startswith("/home/pepa") and used[-1].endswith("deepseek-v4.1-flash"), "footer below the editor, model on the right", prompt)

lines, _, prompt = run(demo, "C", [b"hola\r", b"n\r", b"\r", b"x\r", b"\x04"])
text = "\n".join(lines)
ptext = "\n".join(prompt)
check("Hola, ?que tal? Miro los archivos." in text, "LANG=C: accents become ASCII", lines)
check("─" not in ptext and "-" * 20 in ptext, "LANG=C: rules become '-'", prompt)
check("TinyShell demo - opencode-go - /ayuda" in text, "LANG=C: the separator becomes '-'", lines)
check("(rechazado)" in text, "anything but s rejects", lines)
print("tui: %d failures" % failures)
sys.exit(1 if failures else 0)
