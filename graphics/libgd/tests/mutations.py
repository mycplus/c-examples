"""Deliberate mistakes: put each one into a copy of src/imgtools.c, build the
unit tests against it, and check that they fail.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Needs LibGD (pkg-config gdlib). Exits with status 1 if a mistake goes unnoticed.
"""
from __future__ import annotations

import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import tempfile

EXAMPLE = pathlib.Path(__file__).resolve().parent.parent

MUTATIONS = [
    ("thumbnail size truncated, not rounded", "int small_side = (int)((b * max_side + a / 2) / a);",
     "int small_side = (int)((b * max_side) / a);"),
    ("small images enlarged to the box", "if (w <= max_side && h <= max_side) {", "if (w < 0) {"),
    ("thin images get a zero-pixel side", "if (small_side < 1)\n        small_side = 1;", ""),
    ("PNG header width and height swapped", "*width = (int)w;\n            *height = (int)h;",
     "*width = (int)h;\n            *height = (int)w;"),
    ("JPEG header width and height swapped", "*h = (int)be16(b + 1);\n            *w = (int)be16(b + 3);",
     "*w = (int)be16(b + 1);\n            *h = (int)be16(b + 3);"),
    ("wrong FNV prime", "h *= 16777619u;", "h *= 16777617u;"),
    ("identicon mirrored onto the wrong column", "cells[row][4 - col] = on;", "cells[row][3 - col] = on;"),
    ("identicon reads the wrong hash bits", "(int)(h >> (row * 3 + col) & 1u)", "(int)(h >> (row * 5 + col) & 1u)"),
    ("bar heights truncated, not rounded", "return (int)((value * plot_h + max / 2) / max);",
     "return (int)((value * plot_h) / max);"),
    ("bars one pixel too tall", "gdImageFilledRectangle(im, bx, base - bh, bx + bw - 1, base - 1, blue);",
     "gdImageFilledRectangle(im, bx, base - bh - 1, bx + bw - 1, base - 1, blue);"),
    ("bars one pixel too wide", "gdImageFilledRectangle(im, bx, base - bh, bx + bw - 1, base - 1, blue);",
     "gdImageFilledRectangle(im, bx, base - bh, bx + bw, base - 1, blue);"),
    ("unknown extension saved as JPEG", "if (im == NULL || fmt == IMG_UNKNOWN)", "if (im == NULL)"),
]


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    flags = shlex.split(subprocess.run(["pkg-config", "--cflags", "--libs", "gdlib"],
                                       capture_output=True, text=True, check=True).stdout)
    source = (EXAMPLE / "src" / "imgtools.c").read_text()
    missed = 0
    print(f"{'mistake':<42} result")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        shutil.copy(EXAMPLE / "src" / "imgtools.h", work / "imgtools.h")
        for name, old, new in MUTATIONS:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            (work / "imgtools.c").write_text(source.replace(old, new, 1))
            exe = work / "test_imgtools"
            subprocess.run([cc, "-std=c11", "-w", "-g", f"-I{work}", str(work / "imgtools.c"),
                            str(EXAMPLE / "tests" / "test_imgtools.c"), "-o", str(exe), *flags], check=True)
            try:
                run = subprocess.run([str(exe), str(EXAMPLE / "data" / "downloads.txt")], cwd=work,
                                     capture_output=True, text=True, timeout=60, check=False)
            except subprocess.TimeoutExpired:
                print(f"{name:<42} caught: timed out")
                continue
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
