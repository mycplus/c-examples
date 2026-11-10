"""Deliberate mistakes: put each one into a copy of src/btree.c, build the
test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_btree.c fail; if one does not, the script
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
    ("split leaves the median in the left node",
     "    y->n = t - 1;\n\n    /* Open a gap", "    y->n = t;\n\n    /* Open a gap"),
    ("split moves the wrong children",
     "memcpy(z->child, y->child + t,", "memcpy(z->child, y->child + t - 1,"),
    ("only the root is split before inserting",
     "        if (x->child[i]->n == 2 * t - 1) {", "        if (0) {"),
    ("search finds the first key > k, not >= k",
     "if (x->keys[mid] < key)", "if (x->keys[mid] <= key)"),
    ("deletion enters a child with only t-1 keys",
     "x = x->child[fill_child(x, i, t)];", "x = x->child[i];"),
    ("predecessor copied up but not deleted",
     "                key = x->keys[i];\n                x = y;", "                return true;"),
    ("borrowing from the left forgets the child pointer",
     "        c->child[0] = s->child[s->n];", "        (void)0;"),
    ("merge with the left sibling enters the wrong child",
     "        merge_children(x, i - 1);\n        --i;", "        merge_children(x, i - 1);"),
    ("an empty root is kept",
     "    if (r->n == 0) {", "    if (0) {"),
]


def sanitizer(stderr: str) -> str:
    """The error type and the first frame in btree.c, without paths or addresses."""
    m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)", stderr)
    if m is None:
        m2 = re.search(r"btree\.c:(\d+):\d+: runtime error: ([^\n]*)", stderr)
        return f"UBSan at btree.c:{m2.group(1)}: {m2.group(2)}" if m2 else ""
    frame = re.search(r"#\d+ 0x[0-9a-f]+ in (\w+) [^\n]*btree\.c:(\d+)", stderr)
    where = f" in {frame.group(1)} (btree.c:{frame.group(2)})" if frame else ""
    return f"AddressSanitizer: {m.group(1)}{where}"


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "btree.c").read_text()
    missed = 0
    print(f"{'mistake':<52} result")
    with tempfile.TemporaryDirectory() as tmp:
        for name, old, new in MUTATIONS:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            mutant = pathlib.Path(tmp) / "btree.c"
            mutant.write_text(source.replace(old, new, 1))
            exe = pathlib.Path(tmp) / "test_btree"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            f"-I{EXAMPLE / 'src'}", str(mutant),
                            str(EXAMPLE / "tests" / "test_btree.c"), "-o", str(exe)], check=True)
            try:
                env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
                run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120,
                                     check=False, env=env)
            except subprocess.TimeoutExpired:
                print(f"{name:<52} caught: the tests did not finish in 120 s")
                continue
            if run.returncode == 0:
                print(f"{name:<52} MISSED")
                missed += 1
            elif sanitizer(run.stderr):
                print(f"{name:<52} caught: {sanitizer(run.stderr)}")
            elif run.returncode < 0:
                print(f"{name:<52} caught: the tests crashed (signal {-run.returncode})")
            else:
                first = next((line.strip() for line in run.stdout.splitlines()
                              if line.startswith(("FAILED", "  t="))), "a check failed")
                print(f"{name:<52} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
