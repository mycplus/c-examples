"""Compare seqalign's scores with Biopython's PairwiseAligner, an independent
implementation, on random protein pairs.

Run from the example directory:  python3 tests/compare_biopython.py build/seqalign
Needs Biopython (pip install biopython). Exits with status 1 on any
disagreement.

Biopython charges open_gap_score for the first position of a gap and
extend_gap_score for each further one; seqalign charges open once plus extend
for every position. The same gap costs are therefore passed to Biopython as
open_gap_score = -(open + extend). The last line shows what happens if the
numbers are passed across unchanged.
"""
from __future__ import annotations

import pathlib
import random
import subprocess
import sys
import tempfile

from Bio import Align
from Bio.Align import substitution_matrices

AMINO = "ARNDCQEGHILKMFPSTWYV"
GAPS = [(11, 1), (10, 2), (0, 4), (12, 3), (5, 1)]


def seqalign_score(exe: str, a: str, b: str, mode: str, gap: tuple[int, int], tmp: pathlib.Path) -> int:
    (tmp / "a.fa").write_text(f">a\n{a}\n")
    (tmp / "b.fa").write_text(f">b\n{b}\n")
    args = [exe, "-score", "-open", str(gap[0]), "-extend", str(gap[1])]
    if mode == "local":
        args.append("-local")
    run = subprocess.run(args + [str(tmp / "a.fa"), str(tmp / "b.fa")],
                         capture_output=True, text=True, check=True)
    return int(run.stdout)


def main() -> int:
    exe = sys.argv[1] if len(sys.argv) > 1 else "build/seqalign"
    rng = random.Random(7)
    aligner = Align.PairwiseAligner()
    aligner.substitution_matrix = substitution_matrices.load("BLOSUM62")
    total = agree = unchanged = 0
    with tempfile.TemporaryDirectory() as tmp:
        for _ in range(300):
            a = "".join(rng.choice(AMINO) for _ in range(rng.randint(1, 300)))
            # Half the pairs are related: b is a mutated copy of part of a.
            if rng.random() < 0.5:
                b = "".join(rng.choice(AMINO) for _ in range(rng.randint(1, 300)))
            else:
                b = list(a[rng.randrange(len(a)):])
                for _ in range(len(b) // 5):
                    k = rng.randrange(len(b))
                    b[k:k + 1] = rng.choice([[], [rng.choice(AMINO)], [b[k], rng.choice(AMINO)]])
                b = "".join(b) or "A"
            gap = rng.choice(GAPS)
            for mode in ("global", "local"):
                ours = seqalign_score(exe, a, b, mode, gap, pathlib.Path(tmp))
                aligner.mode = mode
                aligner.open_gap_score = -(gap[0] + gap[1])
                aligner.extend_gap_score = -gap[1]
                theirs = aligner.score(a, b)
                aligner.open_gap_score = -gap[0]
                unchanged += ours == aligner.score(a, b)
                total += 1
                if ours == theirs:
                    agree += 1
                else:
                    print(f"{mode} open={gap[0]} extend={gap[1]}: seqalign {ours}, Biopython {theirs}")
    print(f"alignments where seqalign and Biopython agree on the score: {agree} of {total}")
    print(f"with the gap costs passed to Biopython unchanged: {unchanged} of {total}")
    return 0 if agree == total else 1


if __name__ == "__main__":
    sys.exit(main())
