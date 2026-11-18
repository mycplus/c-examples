"""Deliberate mistakes: put each one into a copy of the library, build the
unit tests against it, run them on the fixture, and check that they fail.

Run from the example directory on Linux:  python3 tests/mutations.py [C compiler]
Exits with status 1 if any mistake goes unnoticed.
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
SOURCES = ["fs_common.c", "fs_posix.c", "walk.c"]

# (description, file, text, replacement)
MUTATIONS = [
    ("join allows one byte too many", "fs_common.c", "if (d + slash + n + 1 > cap)", "if (d + slash + n > cap)"),
    ("join always adds a slash", "fs_common.c", "bool slash = d > 0 &&", "bool slash = 1 ||"),
    ("files sorted before directories", "fs_common.c", "return yd - xd;", "return xd - yd;"),
    ("readdir returns \".\" and \"..\"", "fs_posix.c",
     'if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)\n            continue;', ""),
    ("stat on the bare name, not inside the directory", "fs_posix.c",
     "fstatat(dirfd(d->dir), de->d_name, &st, AT_SYMLINK_NOFOLLOW)", "lstat(de->d_name, &st)"),
    ("symbolic links are followed", "fs_posix.c", "&st, AT_SYMLINK_NOFOLLOW)", "&st, 0)"),
    ("directory size taken from stat", "fs_posix.c",
     "e->size = e->kind == FS_DIR ? 0 : (long long)st.st_size;", "e->size = (long long)st.st_size;"),
    ("hidden test inverted", "fs_posix.c", "e->hidden = de->d_name[0] == '.';", "e->hidden = de->d_name[0] != '.';"),
    ("walk does not restore the path", "walk.c", "            path[len] = '\\0';            /* back to this directory's path */\n", ""),
    ("directory totals not added up", "walk.c", "        dir->size += c->size;\n", ""),
    ("hidden filter ignored", "walk.c", "if (list[i].hidden && !with_hidden)", "if (0)"),
    ("depth limit ignored", "walk.c", "c->kind == FS_DIR && depth_left != 0", "c->kind == FS_DIR"),
    ("comma every three digits from the left", "walk.c", "(len - i) % 3 == 0", "i % 3 == 0"),
]


def build(cc: str, srcdir: pathlib.Path, main: pathlib.Path, exe: pathlib.Path) -> None:
    subprocess.run([cc, "-std=c11", "-w", "-g", "-fsanitize=address,undefined", f"-I{srcdir}",
                    *[str(srcdir / s) for s in SOURCES], str(main), "-o", str(exe)], check=True)


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    missed = 0
    print(f"{'mistake':<48} result")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        good = work / "good"
        shutil.copytree(EXAMPLE / "src", good)
        build(cc, good, EXAMPLE / "src" / "make_fixture.c", work / "make_fixture")
        subprocess.run([str(work / "make_fixture"), str(work / "fixture")], check=True)
        for i, (name, file, old, new) in enumerate(MUTATIONS):
            mutant = work / f"m{i}"
            shutil.copytree(EXAMPLE / "src", mutant)
            source = (mutant / file).read_text()
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            (mutant / file).write_text(source.replace(old, new, 1))
            exe = mutant / "test_fs"
            build(cc, mutant, EXAMPLE / "tests" / "test_fs.c", exe)
            env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
            try:
                run = subprocess.run([str(exe), str(work / "fixture"), str(mutant / "scratch")],
                                     capture_output=True, text=True, timeout=30, check=False, env=env)
            except subprocess.TimeoutExpired:
                print(f"{name:<48} caught: the tests did not finish in 30 s")
                continue
            if run.returncode == 0:
                print(f"{name:<48} MISSED")
                missed += 1
                continue
            m = re.search(r"ERROR: AddressSanitizer: ([\w-]+)|runtime error: ([^\n]+)", run.stderr)
            first = next((l for l in run.stderr.splitlines() if l.startswith("FAILED")), None)
            if first:
                result = re.sub(r"FAILED line \d+: ", "", first)
            elif m:
                result = "sanitizer: " + (m.group(1) or m.group(2))
            else:
                result = f"crashed (status {run.returncode})"
            print(f"{name:<48} caught: {result}")
    print(f"mistakes the tests miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
