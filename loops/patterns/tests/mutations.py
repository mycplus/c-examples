"""Deliberate mistakes: put each one into a copy of src/patterns.c, build the
unit tests against it, and check that they fail.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Exits with status 1 if any mistake goes unnoticed.
"""
from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

EXAMPLE = pathlib.Path(__file__).resolve().parent.parent

MUTATIONS = [
    ("pyramid rows get 2i stars", "repeat(out, '*', 2 * i - 1);\n    fputc('\\n', out);\n}",
     "repeat(out, '*', 2 * i);\n    fputc('\\n', out);\n}"),
    ("pyramid indent one short", "repeat(out, ' ', n - i);\n    repeat(out, '*', 2 * i - 1);",
     "repeat(out, ' ', n - i - 1);\n    repeat(out, '*', 2 * i - 1);"),
    ("diamond repeats its middle row", "for (int i = n - 1; i >= 1; i--)\n        pyramid_row(out, n, i);\n}\n\nstatic void hollow_diamond_row",
     "for (int i = n; i >= 1; i--)\n        pyramid_row(out, n, i);\n}\n\nstatic void hollow_diamond_row"),
    ("hollow pyramid base left open", "k == 1 || k == 2 * i - 1 || i == n ? '*' : ' '", "k == 1 || k == 2 * i - 1 ? '*' : ' '"),
    ("hollow square misses the bottom edge", "i == 1 || i == n || j == 1 || j == n", "i == 1 || j == 1 || j == n"),
    ("hourglass repeats its waist", "for (int i = 2; i <= n; i++)\n        pyramid_row(out, n, i);", "for (int i = 1; i <= n; i++)\n        pyramid_row(out, n, i);"),
    ("Floyd's triangle restarts every row", "int width = digits((long long)n * (n + 1) / 2), next = 1;\n    for (int i = 1; i <= n; i++) {",
     "int width = digits((long long)n * (n + 1) / 2), next = 1;\n    for (int i = 1; i <= n; i++) {\n        next = 1;"),
    ("Pascal entries: multiply before dividing", "unsigned long long q = c / ((unsigned long long)j / g), f = m / g;",
     "unsigned long long q = c * m / (unsigned long long)j, f = 1; (void)g;"),
    ("Pascal entries ignore symmetry wrongly", "if (k > r - k)\n        k = r - k;", "if (k > r - k)\n        k = r - k - 1;"),
    ("number triangle repeats the row number", 'fprintf(out, j == 1 ? "%d" : " %d", j);', 'fprintf(out, j == 1 ? "%d" : " %d", i);'),
]


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "patterns.c").read_text()
    missed = 0
    print(f"{'mistake':<42} result")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        shutil.copy(EXAMPLE / "src" / "patterns.h", work / "patterns.h")
        for name, old, new in MUTATIONS:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            (work / "patterns.c").write_text(source.replace(old, new, 1))
            exe = work / "test_patterns"
            subprocess.run([cc, "-std=c11", "-w", "-g", "-fsanitize=address,undefined", f"-I{work}",
                            str(work / "patterns.c"), str(EXAMPLE / "tests" / "test_patterns.c"), "-o", str(exe)],
                           check=True)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60, check=False)
            if run.returncode == 0:
                print(f"{name:<42} MISSED")
                missed += 1
                continue
            first = next((l for l in run.stderr.splitlines() if l.startswith("FAILED")), "a check failed")
            first = re.sub(r"FAILED line \d+: ", "", first)
            print(f"{name:<42} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
