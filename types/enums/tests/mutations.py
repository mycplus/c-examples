"""Deliberate mistakes: put each one into a copy of src/order.c, build the
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
    ("valid range starts at 0", "return v >= ORDER_PENDING &&", "return v >= 0 &&"),
    ("valid range misses the last status", "v <= ORDER_CANCELLED;\n}", "v < ORDER_CANCELLED;\n}"),
    ("two statuses share a name", 'case ORDER_DELIVERED: return "delivered";', 'case ORDER_DELIVERED: return "shipped";'),
    ("parse skips the last status", "for (int v = ORDER_PENDING; v <= ORDER_CANCELLED; v++) {",
     "for (int v = ORDER_PENDING; v < ORDER_CANCELLED; v++) {"),
    ("parse accepts a prefix", "if (strcmp(text, order_status_name((order_status)v)) == 0) {",
     "if (strncmp(text, order_status_name((order_status)v), strlen(text)) == 0) {"),
    ("shipped orders can be cancelled", "case ORDER_SHIPPED:   return to == ORDER_DELIVERED;",
     "case ORDER_SHIPPED:   return to == ORDER_DELIVERED || to == ORDER_CANCELLED;"),
    ("an order can skip payment", "case ORDER_PENDING:   return to == ORDER_PAID || to == ORDER_CANCELLED;",
     "case ORDER_PENDING:   return to != ORDER_PENDING;"),
    ("invalid status moves anywhere", "case ORDER_CANCELLED: return false;\n    }\n    return false;",
     "case ORDER_CANCELLED: return false;\n    }\n    return true;"),
]


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "order.c").read_text()
    missed = 0
    print(f"{'mistake':<40} result")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        shutil.copy(EXAMPLE / "src" / "order.h", work / "order.h")
        for name, old, new in MUTATIONS:
            if source.count(old) < 1:
                print(f"{name}: text to replace not found")
                return 1
            (work / "order.c").write_text(source.replace(old, new, 1))
            exe = work / "test_enums"
            subprocess.run([cc, "-std=c11", "-w", "-g", "-fsanitize=address,undefined", f"-I{work}",
                            str(work / "order.c"), str(EXAMPLE / "tests" / "test_enums.c"), "-o", str(exe)],
                           check=True)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30, check=False)
            if run.returncode == 0:
                print(f"{name:<40} MISSED")
                missed += 1
            else:
                first = next((l for l in run.stderr.splitlines() if l.startswith("FAILED")), "a check failed")
                first = re.sub(r"FAILED line \d+: ", "", first)
                print(f"{name:<40} caught: {first}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
