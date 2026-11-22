"""Deliberate mistakes: put each one into a copy of the library, build the
test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_mst.c fail (except the listed
equivalent change, which must pass); if one does not, the script exits with
status 1. Mutants are built with AddressSanitizer and UndefinedBehaviorSanitizer,
so a memory error or an overflow is reported by type and place.
"""
from __future__ import annotations

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

EXAMPLE = pathlib.Path(__file__).resolve().parent.parent

# (description, text, replacement), all in src/mst.c
MUTATIONS = [
    ("cycles not rejected",
     "if (ds_union(&ds, edges[i].u, edges[i].v)) {", "if (ds_union(&ds, edges[i].u, edges[i].v) || 1) {"),
    ("find follows only one parent link",
     "    while (ds->parent[x] != x) {\n        ds->parent[x] = ds->parent[ds->parent[x]];",
     "    if (ds->parent[x] != x) {\n        ds->parent[x] = ds->parent[ds->parent[x]];"),
    ("path halving points a vertex at itself",
     "ds->parent[x] = ds->parent[ds->parent[x]];", "ds->parent[x] = x;"),
    ("edges sorted most expensive first",
     "return (a->weight > b->weight) - (a->weight < b->weight);",
     "return (a->weight < b->weight) - (a->weight > b->weight);"),
    ("comparator written as a subtraction",
     "return (a->weight > b->weight) - (a->weight < b->weight);", "return a->weight - b->weight;"),
    ("cheapest edge skipped",
     "for (size_t i = 0; i < m && taken < n - 1; i++) {", "for (size_t i = 1; i < m && taken < n - 1; i++) {"),
    ("disconnected graph reported as a tree",
     "    return taken == n - 1;", "    return true;"),
    ("cost counts every edge examined",
     "            tree[taken++] = edges[i];\n            *cost += edges[i].weight;\n        }",
     "            tree[taken++] = edges[i];\n        }\n        *cost += edges[i].weight;"),
]

# Union by size only keeps the trees shallow: joining the larger set below
# the smaller one is slower but still correct.
EQUIVALENT = [
    ("union by size reversed",
     "if (ds->size[a] < ds->size[b]) {", "if (ds->size[a] > ds->size[b]) {"),
]


def sanitizer(stderr: str) -> str:
    """The error type and place, without paths or addresses."""
    m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)", stderr)
    if m:
        frame = re.search(r"#\d+ 0x[0-9a-f]+ in (\w+) [^\n]*/(\w+\.c):(\d+)", stderr)
        where = f" in {frame.group(1)} ({frame.group(2)}:{frame.group(3)})" if frame else ""
        return f"AddressSanitizer: {m.group(1)}{where}"
    m = re.search(r"(\w+\.c):(\d+):\d+: runtime error: ([^:\n]+)", stderr)
    return f"UBSan at {m.group(1)}:{m.group(2)}: {m.group(3)}" if m else ""


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    missed = 0
    print(f"{'mistake':<48} result")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        for name, old, new in MUTATIONS + EQUIVALENT:
            for f in ("mst.c", "mst.h"):
                shutil.copy(EXAMPLE / "src" / f, work / f)
            source = (work / "mst.c").read_text()
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            (work / "mst.c").write_text(source.replace(old, new, 1))
            exe = work / "test_mst"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", f"-I{work}", str(work / "mst.c"),
                            str(EXAMPLE / "tests" / "test_mst.c"),
                            "-o", str(exe)], check=True)
            try:
                env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
                run = subprocess.run([str(exe)], capture_output=True,
                                     text=True, errors="replace", timeout=30, check=False, env=env)
            except subprocess.TimeoutExpired:
                print(f"{name:<48} caught: the tests did not finish in 30 s")
                continue
            if run.returncode == 0:
                equivalent = any(name == e[0] for e in EQUIVALENT)
                print(f"{name:<48} {'passes: still correct' if equivalent else 'MISSED'}")
                missed += not equivalent
            elif sanitizer(run.stderr):
                print(f"{name:<48} caught: {sanitizer(run.stderr)}")
            elif run.returncode < 0:
                print(f"{name:<48} caught: the tests crashed (signal {-run.returncode})")
            else:
                first = next((line.strip() for line in run.stdout.splitlines()
                              if line.startswith("FAILED")), "a check failed")
                print(f"{name:<48} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
