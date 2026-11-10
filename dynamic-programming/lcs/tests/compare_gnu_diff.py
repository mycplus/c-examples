"""Compares lcs_diff with GNU diff on random pairs of files.

GNU `diff --minimal` finds a shortest edit script, and a shortest edit script
deletes and inserts exactly n + m - 2 * LCS lines. So the number of changed
lines from both programs must agree on every pair.

Run from the example directory after building:
    python3 tests/compare_gnu_diff.py build/lcs_diff
"""
from __future__ import annotations

import pathlib
import random
import subprocess
import sys
import tempfile

WORDS = ["int x = 0;", "x++;", "return x;", "}", "{", "if (x)", "else", ""]


def changed_lines(cmd: list[str], markers: tuple[str, ...]) -> int:
    out = subprocess.run(cmd, capture_output=True, text=True, check=False).stdout
    return sum(1 for line in out.splitlines() if line.startswith(markers))


def main() -> int:
    ours = sys.argv[1]
    rng = random.Random(11)
    agree = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        a, b = pathlib.Path(tmp) / "a.txt", pathlib.Path(tmp) / "b.txt"
        for _ in range(300):
            a.write_text("".join(rng.choice(WORDS) + "\n" for _ in range(rng.randint(0, 40))))
            b.write_text("".join(rng.choice(WORDS) + "\n" for _ in range(rng.randint(0, 40))))
            mine = changed_lines([ours, str(a), str(b)], ("- ", "+ "))
            gnu = changed_lines(["diff", "--minimal", str(a), str(b)], ("<", ">"))
            total += 1
            agree += mine == gnu
    print(f"random file pairs where lcs_diff and GNU diff --minimal change the same "
          f"number of lines: {agree} of {total}")
    return 0 if agree == total else 1


if __name__ == "__main__":
    sys.exit(main())
