"""Compares this stemmer's output with NLTK's independent Python
implementation, word by word, for the whole test vocabulary.

Run from the example directory after building:
    python3 tests/compare_nltk.py build/stem
NLTK's ORIGINAL_ALGORITHM mode should match --paper, MARTIN_EXTENSIONS should
match the standard mode, and NLTK's default mode is reported for reference.
"""
from __future__ import annotations

import pathlib
import subprocess
import sys

import nltk
from nltk.stem.porter import PorterStemmer

VOC = pathlib.Path(__file__).resolve().parent / "vectors" / "voc.txt"


def ours(stem: str, *flags: str) -> list[str]:
    out = subprocess.run([stem, *flags, str(VOC)], capture_output=True, text=True, check=True)
    return out.stdout.splitlines()


def main() -> int:
    words = VOC.read_text().splitlines()
    paper, standard = ours(sys.argv[1], "--paper"), ours(sys.argv[1])
    ok = True
    for mode, mine in (("ORIGINAL_ALGORITHM", paper), ("MARTIN_EXTENSIONS", standard),
                       ("NLTK_EXTENSIONS", standard)):
        p = PorterStemmer(mode=mode)
        theirs = [p.stem(w, to_lowercase=False) for w in words]
        diff = sum(a != b for a, b in zip(mine, theirs))
        against = "--paper" if mine is paper else "standard"
        print(f"NLTK {nltk.__version__} {mode:<18} vs {against:<8}: {diff} of {len(words)} differ")
        if mode != "NLTK_EXTENSIONS" and diff != 0:
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
