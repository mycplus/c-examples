"""Deliberate mistakes: put each one into a copy of src/lcs.c, build the
test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_lcs.c fail (except the listed equivalent
change, which must pass); if one does not, the script
exits with status 1. Mutants are built with AddressSanitizer and
UndefinedBehaviorSanitizer, so a memory error is reported by type and place
instead of as a bare crash.
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
    ("a match extends the cell above, not the diagonal",
     "AT(i, j) = AT(i - 1, j - 1) + 1;", "AT(i, j) = AT(i - 1, j) + 1;"),
    ("min instead of max of the two neighbours",
     "AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i - 1, j) : AT(i, j - 1);",
     "AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i, j - 1) : AT(i - 1, j);"),
    ("characters compared one position off",
     "else if (a[i - 1] == b[j - 1])\n                AT(i, j)",
     "else if (a[i] == b[j])\n                AT(i, j)"),
    ("the two rows are never swapped",
     "        size_t *t = prev; prev = cur; cur = t;\n", ""),
    ("no room for the terminating NUL",
     "char *out = malloc(len + 1);", "char *out = malloc(len);"),
    ("a literal returned for an empty LCS",
     "    char *out = malloc(len + 1);", "    if (len == 0) { free(L); return \"\"; }\n    char *out = malloc(len + 1);"),
    ("the diff swaps - and +",
     "op[--k] = '+';", "op[--k] = '-';"),
]

# Changing which neighbour the walk-back prefers on a tie gives a different
# LCS of the same length, which is not a bug: the tests accept any LCS. The
# script reports these but does not count them as missed.
EQUIVALENT = [
    ("walk-back prefers left instead of up on a tie",
     "} else if (AT(i - 1, j) >= AT(i, j - 1)) {", "} else if (AT(i - 1, j) > AT(i, j - 1)) {"),
]


def sanitizer(stderr: str) -> str:
    """The error type and the first frame in lcs.c, without paths or addresses."""
    m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)", stderr)
    if m is None:
        m2 = re.search(r"lcs\.c:(\d+):\d+: runtime error: ([^\n]*)", stderr)
        return f"UBSan at lcs.c:{m2.group(1)}: {m2.group(2)}" if m2 else ""
    frame = re.search(r"#\d+ 0x[0-9a-f]+ in (\w+) [^\n]*/lcs\.c:(\d+)", stderr)
    where = f" in {frame.group(1)} (lcs.c:{frame.group(2)})" if frame else ""
    return f"AddressSanitizer: {m.group(1)}{where}"


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "lcs.c").read_text()
    missed = 0
    print(f"{'mistake':<52} result")
    with tempfile.TemporaryDirectory() as tmp:
        for name, old, new in MUTATIONS + EQUIVALENT:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            mutant = pathlib.Path(tmp) / "lcs.c"
            mutant.write_text(source.replace(old, new, 1))
            exe = pathlib.Path(tmp) / "test_lcs"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            f"-I{EXAMPLE / 'src'}", str(mutant),
                            str(EXAMPLE / "tests" / "test_lcs.c"), "-o", str(exe)], check=True)
            try:
                env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
                run = subprocess.run([str(exe)], capture_output=True, text=True, errors="replace",
                                     timeout=120,
                                     check=False, env=env)
            except subprocess.TimeoutExpired:
                print(f"{name:<52} caught: the tests did not finish in 120 s")
                continue
            if run.returncode == 0:
                equivalent = (name, old, new) in EQUIVALENT
                print(f"{name:<52} {'passes: still a correct LCS' if equivalent else 'MISSED'}")
                missed += not equivalent
            elif sanitizer(run.stderr):
                print(f"{name:<52} caught: {sanitizer(run.stderr)}")
            elif run.returncode < 0:
                print(f"{name:<52} caught: the tests crashed (signal {-run.returncode})")
            else:
                first = next((line.strip() for line in run.stdout.splitlines()
                              if line.startswith(("FAILED", "  a="))), "a check failed")
                first = "".join(c if c.isprintable() and c.isascii() else "?" for c in first)
                print(f"{name:<52} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
