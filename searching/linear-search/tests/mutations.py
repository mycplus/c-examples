"""Deliberate mistakes: put each one into a copy of src/linear_search.c,
build the test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_linear_search.c fail (except the listed
equivalent change, which must pass); if one does not, the script exits with
status 1. Mutants are built with AddressSanitizer and UndefinedBehaviorSanitizer,
so a memory error is reported by type and place instead of as a bare crash.
"""
from __future__ import annotations

import os
import pathlib
import re
import subprocess
import sys
import tempfile

EXAMPLE = pathlib.Path(__file__).resolve().parent.parent

MUTATIONS = [
    ("loop runs one element too far (i <= n)",
     "    for (size_t i = 0; i < n; i++)\n        if (a[i] == key)",
     "    for (size_t i = 0; i <= n; i++)\n        if (a[i] == key)"),
    ("sentinel never removed",
     "    a[n - 1] = last;\n", ""),
    ("sentinel taken for a real match",
     "if (i < n - 1 || last == key)", "if (i < n)"),
    ("search_first steps one byte at a time",
     "i++, p += size)", "i++, p++)"),
    ("move-to-front shifts in the wrong direction",
     "for (size_t i = (size_t)at; i > 0; i--)    /* shift the others back */\n            a[i] = a[i - 1];",
     "for (size_t i = 1; i <= (size_t)at; i++)\n            a[i] = a[i - 1];"),
    ("transpose overwrites instead of swapping",
     "        a[at] = a[at - 1];\n", ""),
    ("binary search never moves past mid",
     "lo = mid + 1;", "lo = mid;"),
    ("binary search finds the last match, not the first",
     "if (a[mid] < key)", "if (a[mid] <= key)"),
    ("binary search ignores the last element",
     "size_t lo = 0, hi = n;", "size_t lo = 0, hi = n - 1;"),
]

# (lo + hi) / 2 can only overflow when the array has more than SIZE_MAX / 2
# elements, which no array in memory can have, so this change must pass.
EQUIVALENT = [
    ("binary search midpoint as (lo + hi) / 2",
     "size_t mid = lo + (hi - lo) / 2;", "size_t mid = (lo + hi) / 2;"),
]


def sanitizer(stderr: str) -> str:
    """The error type and the first frame in linear_search.c, without paths or addresses."""
    m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)", stderr)
    if m is None:
        m2 = re.search(r"([\w.]+\.c):(\d+):\d+: runtime error: ([^\n]*?)( 0x[0-9a-f]+)?( for type[^\n]*)?\n", stderr)
        return f"UBSan at {m2.group(1)}:{m2.group(2)}: {m2.group(3)}" if m2 else ""
    frame = re.search(r"#\d+ 0x[0-9a-f]+ in (\w+) [^\n]*/linear_search\.c:(\d+)", stderr)
    where = f" in {frame.group(1)} (linear_search.c:{frame.group(2)})" if frame else ""
    return f"AddressSanitizer: {m.group(1)}{where}"


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "linear_search.c").read_text()
    missed = 0
    print(f"{'mistake':<52} result")
    with tempfile.TemporaryDirectory() as tmp:
        for name, old, new in MUTATIONS + EQUIVALENT:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            mutant = pathlib.Path(tmp) / "linear_search.c"
            mutant.write_text(source.replace(old, new, 1))
            exe = pathlib.Path(tmp) / "test_linear_search"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            f"-I{EXAMPLE / 'src'}", str(mutant),
                            str(EXAMPLE / "tests" / "test_linear_search.c"), "-o", str(exe)], check=True)
            try:
                env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
                run = subprocess.run([str(exe)], capture_output=True, text=True, errors="replace",
                                     timeout=30,
                                     check=False, env=env)
            except subprocess.TimeoutExpired:
                print(f"{name:<52} caught: the tests did not finish in 30 s")
                continue
            if run.returncode == 0:
                equivalent = (name, old, new) in EQUIVALENT
                print(f"{name:<52} {'passes: still correct' if equivalent else 'MISSED'}")
                missed += not equivalent
            elif sanitizer(run.stderr):
                print(f"{name:<52} caught: {sanitizer(run.stderr)}")
            elif run.returncode < 0:
                print(f"{name:<52} caught: the tests crashed (signal {-run.returncode})")
            else:
                first = next((line.strip() for line in run.stdout.splitlines()
                              if line.startswith("FAILED")), "a check failed")
                first = "".join(c if c.isprintable() and c.isascii() else "?" for c in first)
                print(f"{name:<52} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
