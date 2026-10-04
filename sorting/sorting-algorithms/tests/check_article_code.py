#!/usr/bin/env python3
"""Check that sorting.c still holds the same functions as the article folders.

sorting.c copies its insertion, selection, bubble, merge and heap sort
functions from the per-article folders next to it in this repository. This
script compares each copied function with its source, character for character,
so an edit to one copy without the other fails CI. Bubble sort's article code
compares with a plain <; that one comparison is rewritten to SORT_LESS before
comparing. Shell sort and quicksort have no folder in this repository, so
they are not checked here.

Run from anywhere: python3 tests/check_article_code.py
"""
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
HUB = HERE.parent / "src" / "sorting.c"
SORTING = HERE.parent.parent          # c-examples/sorting/

SOURCES = {
    "insertion-sort/src/insertion_sort.c": ["insertion_sort"],
    "selection-sort/src/selection_sort.c": ["selection_sort"],
    "bubble-sort/src/bubble_sort.c": ["bubble_sort"],
    "merge-sort/src/merge_sort.c": ["merge", "merge_sort_rec", "merge_sort"],
    "heap-sort/src/heap_sort.c": ["sift_down", "make_heap_down", "heap_sort"],
}
REWRITES = {
    "bubble-sort/src/bubble_sort.c": [
        ("if (a[j] < a[j - 1])", "if (SORT_LESS(a[j], a[j - 1]))"),
    ],
}


def function(src: str, name: str) -> str | None:
    """Return the definition of name, from its signature line to its closing brace."""
    m = re.search(r"^[^\n;]*\b" + re.escape(name) + r"\([^)]*\)\n\{", src, re.M)
    if m is None:
        return None
    i, depth = m.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(src[i], 0)
        i += 1
    return src[m.start():i]


def main() -> int:
    hub = HUB.read_text()
    failures = 0
    for rel, names in SOURCES.items():
        path = SORTING / rel
        if not path.exists():
            print(f"MISSING {rel}")
            failures += 1
            continue
        src = path.read_text()
        for old, new in REWRITES.get(rel, []):
            src = src.replace(old, new)
        for name in names:
            a, b = function(src, name), function(hub, name)
            if a is None or b is None:
                print(f"NOT FOUND {name} ({'article' if a is None else 'sorting.c'})")
                failures += 1
            elif a != b:
                print(f"DIFFERS   {name}: sorting.c no longer matches {rel}")
                failures += 1
            else:
                print(f"same      {name}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
