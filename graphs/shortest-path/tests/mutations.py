"""Deliberate mistakes: put each one into a copy of the library, build the
test suite against it, and record how the suite fails.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must make tests/test_shortest_path.c fail (except the listed
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

# (description, file, text, replacement)
MUTATIONS = [
    ("Dijkstra accepts negative weights", "shortest_path.c",
     "            if (g->adj[u][k].weight < 0)\n                return false;\n", ""),
    ("Dijkstra does not skip stale heap entries", "shortest_path.c",
     "        if (top.d > dist[top.node])\n            continue;", "        (void)0;"),
    ("Dijkstra relaxes on equal distance (<=)", "shortest_path.c",
     "            if (candidate < dist[e->to]) {", "            if (candidate <= dist[e->to]) {"),
    ("heap pops the larger child", "shortest_path.c",
     "h->items[child + 1].d < h->items[child].d", "h->items[child + 1].d > h->items[child].d"),
    ("Floyd-Warshall keeps the last parallel road", "shortest_path.c",
     "            if (e->weight < dist[u * n + e->to]) {", "            if (1) {"),
    ("Floyd-Warshall adds to \"infinity\"", "shortest_path.c",
     "            if (dist[i * n + k] == DIST_INF)\n                continue;", ""),
    ("Floyd-Warshall records the wrong next hop", "shortest_path.c",
     "next[i * n + j] = next[i * n + k];", "next[i * n + j] = next[k * n + j];"),
    ("negative-cycle test also fires on zero", "shortest_path.c",
     "            if (dist[i * n + i] < 0)\n                return false;\n    }",
     "            if (dist[i * n + i] <= 0)\n                return false;\n    }"),
    ("negative-cycle test only after the last round", "shortest_path.c",
     "        for (int i = 0; i < n; i++)\n            if (dist[i * n + i] < 0)\n                return false;\n    }\n    return true;",
     "    }\n    for (int i = 0; i < n; i++)\n        if (dist[i * n + i] < 0)\n            return false;\n    return true;"),
    ("path from prev is not reversed", "shortest_path.c",
     "    for (int i = 0, j = len - 1; i < j; i++, j--) {", "    for (int i = 0, j = len - 1; i < j && 0; i++, j--) {"),
    ("path from next ignores \"no path\"", "shortest_path.c",
     "    if (next[source * n + target] == -1)\n        return 0;\n", ""),
    ("map loader accepts a zero distance", "graph.c",
     "km <= 0 ||", "km < 0 ||"),
    ("map loader adds roads one way only", "graph.c",
     " || !graph_add_edge(g, v, u, km)", ""),
]

# The heap's first allocation size only changes how often it grows.
EQUIVALENT = [
    ("heap starts with room for 1 item, not 16", "shortest_path.c",
     "size_t cap = h->cap ? h->cap * 2 : 16;", "size_t cap = h->cap ? h->cap * 2 : 1;"),
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
        for name, file, old, new in MUTATIONS + EQUIVALENT:
            for f in ("graph.c", "graph.h", "shortest_path.c", "shortest_path.h"):
                shutil.copy(EXAMPLE / "src" / f, work / f)
            source = (work / file).read_text()
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            (work / file).write_text(source.replace(old, new, 1))
            exe = work / "test_shortest_path"
            subprocess.run([cc, "-std=c11", "-w", "-O1", "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", f"-I{work}", str(work / "graph.c"),
                            str(work / "shortest_path.c"), str(EXAMPLE / "tests" / "test_shortest_path.c"),
                            "-o", str(exe)], check=True)
            try:
                env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
                run = subprocess.run([str(exe), str(EXAMPLE / "data" / "lakeland.txt")], capture_output=True,
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
                first = next((line.strip() for line in run.stderr.splitlines()
                              if line.startswith("FAILED")), "a check failed")
                print(f"{name:<48} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
