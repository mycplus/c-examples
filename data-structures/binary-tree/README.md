# Binary Tree in C

[![binary-tree](https://github.com/mycplus/c-examples/actions/workflows/binary-tree.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/binary-tree.yml)

Companion code for [Binary Trees in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/data-structures/binary-trees/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/bst.c` | The article's binary search tree of `int`: insert, search and remove without recursion, the four traversals, height, size and a non-recursive free |
| `src/bst_shape.c` | Height and mean node depth for random and sorted insertion orders, against the expected depth of a random BST |
| `src/deep_tree.c` | Recursive and iterative height on a chain of N nodes, the shape sorted insertion produces |
| `pitfalls/` | Four mistakes from the article: insert through a by-value pointer, a parent-and-child-only BST check, removing the wrong key, and freeing a node before reading its children |
| `tests/` | Unit and model-based tests, and the expected output of the demo |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- The demo tree's preorder, inorder, postorder and level order, height and size, and every removal case: a leaf, a node with one child on either side, a node with two children and the root.
- An empty tree and a one-node tree, with `INT_MIN` as the key.
- 200,000 random inserts, removes and searches agree with a presence table, including `INT_MIN` and `INT_MAX`; the BST property, the size, sorted in-order output and the two height functions are checked every 1,000 operations.
- The directly linked chain in `deep_tree.c` is exactly the tree that sorted insertion builds.
- On a chain of 1,000,000 nodes, the iterative height, in-order and level-order walks, removal and `bst_free` finish without recursion, with no leaks under LeakSanitizer.
- `bst` prints exactly the output shown in the article.
- The tests and the demo run clean under AddressSanitizer and UndefinedBehaviorSanitizer.
- Each pitfall program behaves as the article describes, and the recursive height overflows the stack on a 1,000,000-node chain under AddressSanitizer.

The build does **not** run `bst_shape`: its depth figures are reproducible, but its timings depend on the machine.

The tests include the `src/` files with `main` compiled out, so they test the same code the article shows rather than a copy.
