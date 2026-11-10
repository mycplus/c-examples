"""Times NLTK's PorterStemmer on the test vocabulary in the same way as
stem_speed.c (median of 5 runs, the copy-and-stem loop timed), and prints
the same FNV-1a hash of the stems.

Usage: python3 bench/nltk_speed.py tests/vectors/voc.txt [ORIGINAL_ALGORITHM|MARTIN_EXTENSIONS|NLTK_EXTENSIONS]
"""
from __future__ import annotations

import statistics
import sys
import time

import nltk
from nltk.stem.porter import PorterStemmer


def main() -> None:
    with open(sys.argv[1], encoding="ascii") as f:
        words = f.read().split("\n")[:-1]
    mode = sys.argv[2] if len(sys.argv) > 2 else PorterStemmer.MARTIN_EXTENSIONS
    stemmer = PorterStemmer(mode=mode)
    stemmer.stem("warmup")
    h = 2166136261
    for w in words:
        for ch in stemmer.stem(w, to_lowercase=False) + "\n":
            h = ((h ^ ord(ch)) * 16777619) & 0xFFFFFFFF
    times = []
    for _ in range(5):
        start = time.perf_counter()
        for w in words:
            stemmer.stem(w, to_lowercase=False)
        times.append(time.perf_counter() - start)
    ns = statistics.median(times) / len(words) * 1e9
    print(f"NLTK {nltk.__version__} ({mode}): {len(words)} words, {ns:.0f} ns per word, "
          f"hash {h:08x}")


if __name__ == "__main__":
    main()
