"""Deliberate mistakes: put each one into a copy of src/porter.c, build it,
stem the test vocabulary, and count the words whose stems change.

Run from the example directory:  python3 tests/mutations.py [C compiler]
Every mistake must change at least one stem, or the vocabulary test would
not catch it, and the script exits with status 1.
"""
from __future__ import annotations

import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
EXAMPLE = HERE.parent
VOC = HERE / "vectors" / "voc.txt"
EXPECTED = (HERE / "vectors" / "standard_output.txt").read_text().splitlines()

LONGEST = """    if (best == NULL)
        return;
    w->j = w->k - best_len;"""

# A version of apply_longest that falls back to shorter suffixes when the
# longest one's condition fails.
FALLBACK = """static void apply_longest(word *w, const rule *rules, size_t count, unsigned min_m)
{
    for (size_t len = 8; len > 0; --len)
        for (size_t i = 0; i < count; ++i) {
            if (strlen(rules[i].suffix) != len ||
                (rules[i].only >= 0 && rules[i].only != (int)w->mode) ||
                !ends_with(w, rules[i].suffix))
                continue;
            if (strcmp(rules[i].suffix, "ion") == 0 &&
                (w->j == 0 || (w->b[w->j - 1] != 's' && w->b[w->j - 1] != 't')))
                continue;
            if (porter_measure(w->b, w->j) > min_m) {
                replace_suffix(w, rules[i].replacement);
                return;
            }
        }
}

static void unused_apply_longest(word *w, const rule *rules, size_t count, unsigned min_m)"""

MUTATIONS = [
    ("y is always a consonant",
     "return i == 0 || !is_consonant(b, i - 1);", "return true;"),
    ("y is always a vowel",
     "return i == 0 || !is_consonant(b, i - 1);", "return false;"),
    ("falls back to a shorter suffix",
     "static void apply_longest(word *w, const rule *rules, size_t count, unsigned min_m)",
     FALLBACK),
    ("measures the whole word, not the stem",
     "if (porter_measure(w->b, w->j) > min_m)", "if (porter_measure(w->b, w->k) > min_m)"),
    ("step 4 needs m > 0 instead of m > 1",
     "COUNT(step4_rules), 1)", "COUNT(step4_rules), 0)"),
    ("-ion without the s or t condition",
     "if (strcmp(best->suffix, \"ion\") == 0 &&", "if (0 &&"),
    ("*o forgets the w, x, y exception",
     "return c != 'w' && c != 'x' && c != 'y';", "return true;"),
    ("step 1b skips the tidy-up",
     "    truncate_to(w, w->j);\n\n    /* Tidy up", "    truncate_to(w, w->j);\n    return;\n\n    /* Tidy up"),
    ("step 3 runs before step 2",
     ("case 3: apply_longest(&w, step2_rules, COUNT(step2_rules), 0); break;\n"
      "        case 4: apply_longest(&w, step3_rules, COUNT(step3_rules), 0); break;"),
     ("case 3: apply_longest(&w, step3_rules, COUNT(step3_rules), 0); break;\n"
      "        case 4: apply_longest(&w, step2_rules, COUNT(step2_rules), 0); break;")),
]


def main() -> int:
    cc = sys.argv[1] if len(sys.argv) > 1 else "cc"
    source = (EXAMPLE / "src" / "porter.c").read_text()
    missed = 0
    print(f"{'mistake':<40} {'stems changed':>13}  example")
    with tempfile.TemporaryDirectory() as tmp:
        for name, old, new in MUTATIONS:
            if old not in source:
                print(f"{name}: text to replace not found")
                return 1
            mutant = pathlib.Path(tmp) / "porter.c"
            mutant.write_text(source.replace(old, new, 1))
            exe = pathlib.Path(tmp) / "stem"
            subprocess.run([cc, "-std=c11", "-w", "-O2", f"-I{EXAMPLE / 'src'}", str(mutant),
                            str(EXAMPLE / "src" / "stem.c"), "-o", str(exe)], check=True)
            out = subprocess.run([str(exe), str(VOC)], capture_output=True, text=True,
                                 check=True).stdout.splitlines()
            words = VOC.read_text().splitlines()
            diffs = [(w, e, o) for w, e, o in zip(words, EXPECTED, out) if e != o]
            example = f"{diffs[0][0]}: {diffs[0][1]} -> {diffs[0][2]}" if diffs else "-"
            print(f"{name:<40} {len(diffs):>13}  {example}")
            missed += not diffs
    print(f"mistakes the vocabulary test would miss: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
