# Shortest Path in C: Dijkstra and Floyd-Warshall

[![shortest-path](https://github.com/mycplus/c-examples/actions/workflows/shortest-path.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/shortest-path.yml)

Companion code for [Shortest Path in C: Dijkstra and Floyd–Warshall with a Route Planner](https://www.mycplus.com/computer-science/algorithms/shortest-path/) on MYCPLUS. A small route planner reads a road map from a text file and finds the shortest route between two towns with either algorithm. The same planner is written in C11 and in C++17, and both print identical output.

| File | What it is |
| --- | --- |
| `src/graph.h`, `src/graph.c` | A weighted graph as adjacency lists, with town names, and a map loader that reports the line number of any mistake |
| `src/shortest_path.h`, `src/shortest_path.c` | Dijkstra's algorithm with a binary heap, Floyd-Warshall with a next-hop table, negative-cycle detection, and route reconstruction |
| `src/route.c` | The route planner: `route MAP FROM TO [dijkstra\|floyd]`, `route MAP --from TOWN`, `route MAP --table` |
| `src/trace.c` | Prints every step: `trace MAP dijkstra FROM` and `trace MAP floyd` |
| `cpp/route.cpp` | The same planner in C++17 with `std::priority_queue`, `std::vector` and `std::unordered_map` |
| `data/lakeland.txt` | A fictional map of 13 towns and 19 roads, including an island with no road to the mainland |
| `data/four_towns.txt` | A four-town map small enough to follow Floyd-Warshall by hand |
| `pitfalls/negative_edge.c` | Dijkstra gives wrong answers when an edge is negative (an electric van gaining charge downhill) |
| `pitfalls/magic_infinity.c` | Using 9999 or `INT_MAX` to mean "no road" |
| `pitfalls/loop_order.c` | Floyd-Warshall with the `k` loop on the inside |
| `bench/timing.c` | All-pairs timing: Dijkstra from every town versus one Floyd-Warshall run, on sparse and dense graphs |
| `tests/` | Unit tests, 4,000 random graphs checked against Bellman-Ford, expected outputs, and mutation tests |
| `run_all.sh`, `run_all.ps1` | One-command verification for Ubuntu/WSL and Windows (MSVC); each prints a Markdown report |

Exit status of `route`: 0 when a route is found, 1 when no road connects the two towns, 2 for bad input.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/route data/lakeland.txt Ashford Larkspur
./build/route data/lakeland.txt Ashford Larkspur floyd
```

Map files have one road per line, `Town A,Town B,distance`, with whole-number distances. Blank lines and lines starting with `#` are ignored, and every road runs both ways.

## What the build checks

- Compiles with GCC and Clang under C11 and C17 (and C++17) with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- Dijkstra and Floyd-Warshall agree with Bellman-Ford on 4,000 random graphs with positive, zero and negative weights, and Floyd-Warshall reports every negative cycle Bellman-Ford finds. Every reconstructed route is walked road by road and must add up to the reported distance.
- Dijkstra settles each reachable node exactly once, which catches heap mistakes that still produce correct distances.
- The C and C++ planners print exactly the same output for the same input.
- The programs run clean under AddressSanitizer, UndefinedBehaviorSanitizer and Valgrind.
- `tests/mutations.py` puts 13 deliberate mistakes into the library and confirms the tests catch every one.

`bench/timing.c` is built but not run by `ctest`, because timings depend on the machine.
