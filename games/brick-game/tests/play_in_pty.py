"""Plays the game in a pseudo-terminal: presses keys, quits with q, then
interrupts a second game with Ctrl+C, and checks that both runs exit with
status 0, draw the first frame in full and restore the cursor.

Usage: python3 tests/play_in_pty.py path/to/brick   (Linux and macOS only)
"""
import os
import pty
import select
import sys
import time

ESC = "\x1b"


def play(program, keys, timeout=10.0):
    pid, fd = pty.fork()
    if pid == 0:
        os.execv(program, [program, "expert"])
    output, start, sent = b"", time.monotonic(), 0
    while time.monotonic() - start < timeout:
        elapsed = time.monotonic() - start
        if sent < len(keys) and elapsed >= keys[sent][0]:
            os.write(fd, keys[sent][1])
            sent += 1
        ready, _, _ = select.select([fd], [], [], 0.05)
        if ready:
            try:
                chunk = os.read(fd, 65536)
            except OSError:          # Linux reports EIO once the child exits
                break
            if not chunk:
                break
            output += chunk
    else:
        os.kill(pid, 9)
    _, status = os.waitpid(pid, 0)
    return output.decode("latin-1"), os.waitstatus_to_exitcode(status)


def main():
    program = sys.argv[1]
    runs = {
        "arrow keys, then q": [(0.5, b"\x1b[D"), (0.7, b"\x1b[C"), (0.9, b"d"), (1.2, b"q")],
        "Ctrl+C": [(1.0, b"\x03")],
    }
    failed = False
    for name, keys in runs.items():
        text, code = play(program, keys)
        checks = {
            "exit status 0": code == 0,
            "first frame drawn in full": text.startswith(ESC + "[?25l" + ESC + "[H" + ESC + "[2J"),
            "cursor restored": (ESC + "[?25h") in text,
            "final score printed": "Final score:" in text,
        }
        for what, ok in checks.items():
            print(f"{name}: {what}: {'ok' if ok else 'FAILED'}")
            failed |= not ok
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
