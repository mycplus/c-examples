"""Deliberate mistakes: put each one into a copy of src/align.c, build the
test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_align.c fail (except the listed equivalent
changes, which must pass); if one does not, the script exits with status 1.
Mutants are built with AddressSanitizer and UndefinedBehaviorSanitizer, so a
memory error is reported by type and place instead of as a bare crash.
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
    ("local cells not floored at zero",
     "                diag = 0;                  /* start a new local alignment here */", "                ;"),
    ("extending a gap charged as opening one",
     "AT(t->X, i - 1, j) - ext,", "AT(t->X, i - 1, j) - open,"),
    ("the gap-open cost never charged",
     "const int open = sc->gap_open + sc->gap_extend, ext", "const int open = sc->gap_extend, ext"),
    ("global edge gaps without the opening cost",
     "-(sc->gap_open + (int)i * ext) : NEG", "-((int)i * ext) : NEG"),
    ("the two letters looked up in the wrong order",
     "AT(t->M, i, j) = sc->table[a[i - 1] * width + b[j - 1]] + diag;",
     "AT(t->M, i, j) = sc->table[b[j - 1] * width + a[i - 1]] + diag;"),
    ("local answer read from the bottom-right corner",
     "if (t.M[i * t.cols + j] > out->score) {", "if (i == t.n && j == t.m) {"),
    ("traceback buffer one column short",
     "size_t cap = i + j, len = 0;", "size_t cap = i + j - 1, len = 0;"),
    ("top row letter taken one position off",
     "top[len] = s1[i - 1];\n            bottom[len++] = s2[j - 1];",
     "top[len] = s1[i];\n            bottom[len++] = s2[j - 1];"),
    ("local traceback runs past the start",
     "if (local && v == 0)", "if (local && v < 0)"),
    ("align_score never swaps its gap rows",
     "        tmp = pX, pX = cX, cX = tmp;\n", ""),
]

# Changes that pick a different alignment with the same score. The tests
# accept any optimal alignment, so these must pass. The script reports them
# but does not count them as missed.
EQUIVALENT = [
    ("global traceback prefers a gap to a pair on a tie",
     "state = out->score == t.M[c] ? STATE_M : out->score == t.X[c] ? STATE_X : STATE_Y;",
     "state = out->score == t.X[c] ? STATE_X : out->score == t.M[c] ? STATE_M : STATE_Y;"),
    # Two adjacent gaps can always be written in either order for the same
    # score, so forbidding one order loses nothing.
    ("a gap in s1 never directly after a gap in s2",
     "AT(t->Y, i - 1, j) - open);", "NEG);"),
]


def sanitizer(stderr: str) -> str:
    """The error type and the first frame in align.c, without paths or addresses."""
    m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)", stderr)
    if m is None:
        m2 = re.search(r"align\.c:(\d+):\d+: runtime error: ([^\n]*)", stderr)
        return f"UBSan at align.c:{m2.group(1)}: {m2.group(2)}" if m2 else ""
    frame = re.search(r"#\d+ 0x[0-9a-f]+ in (\w+) [^\n]*/align\.c:(\d+)", stderr)
    where = f" in {frame.group(1)} (align.c:{frame.group(2)})" if frame else ""
    return f"AddressSanitizer: {m.group(1)}{where}"


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "align.c").read_text()
    missed = 0
    print(f"{'mistake':<52} result")
    with tempfile.TemporaryDirectory() as tmp:
        for name, old, new in MUTATIONS + EQUIVALENT:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            mutant = pathlib.Path(tmp) / "align.c"
            mutant.write_text(source.replace(old, new, 1))
            exe = pathlib.Path(tmp) / "test_align"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            f"-I{EXAMPLE / 'src'}", str(mutant),
                            str(EXAMPLE / "src" / "blosum.c"),
                            str(EXAMPLE / "tests" / "test_align.c"), "-o", str(exe)], check=True)
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
                print(f"{name:<52} {'passes: still an optimal alignment' if equivalent else 'MISSED'}")
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
