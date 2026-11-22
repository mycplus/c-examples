// mst.cpp - Kruskal's algorithm in C++17, built from the same three pieces
// as the C version: sort the edges, a disjoint-set class, the greedy loop.
//   mst_cpp           prints the cheapest network for the six offices
//   mst_cpp --test    checks Kruskal against a brute force on random graphs
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

struct Edge {
    int u, v, weight;
};

// Piece 1: which vertices are already connected.
class DisjointSet {
public:
    explicit DisjointSet(int n) : parent_(static_cast<std::size_t>(n)), size_(static_cast<std::size_t>(n), 1)
    {
        std::iota(parent_.begin(), parent_.end(), 0);
    }

    int find(int x)
    {
        while (parent_[idx(x)] != x) {
            parent_[idx(x)] = parent_[idx(parent_[idx(x)])];   // path halving
            x = parent_[idx(x)];
        }
        return x;
    }

    bool unite(int a, int b)   // false if a and b were already connected
    {
        a = find(a);
        b = find(b);
        if (a == b)
            return false;
        if (size_[idx(a)] < size_[idx(b)])
            std::swap(a, b);
        parent_[idx(b)] = a;
        size_[idx(a)] += size_[idx(b)];
        return true;
    }

private:
    static std::size_t idx(int x) { return static_cast<std::size_t>(x); }
    std::vector<int> parent_, size_;
};

struct Tree {
    std::vector<Edge> edges;
    long long cost = 0;
};

// Pieces 2 and 3: sort, then take every edge that joins two separate parts.
// std::nullopt means the graph is not connected.
std::optional<Tree> kruskal(int n, std::vector<Edge> edges)
{
    std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return std::tie(a.weight, a.u, a.v) < std::tie(b.weight, b.u, b.v);
    });
    DisjointSet sets(n);
    Tree tree;
    for (const Edge& e : edges) {
        if (static_cast<int>(tree.edges.size()) == n - 1)
            break;
        if (sets.unite(e.u, e.v)) {
            tree.edges.push_back(e);
            tree.cost += e.weight;
        }
    }
    if (n > 0 && static_cast<int>(tree.edges.size()) != n - 1)
        return std::nullopt;
    return tree;
}

// The brute force: every subset of exactly n - 1 edges, as a bit mask.
std::optional<long long> brute_force(int n, const std::vector<Edge>& edges)
{
    if (n <= 1)
        return 0;
    std::optional<long long> best;
    const std::uint32_t subsets = 1u << static_cast<std::uint32_t>(edges.size());
    for (std::uint32_t mask = 0; mask < subsets; ++mask) {
        int count = 0;
        for (std::uint32_t m = mask; m != 0; m &= m - 1)
            ++count;
        if (count != n - 1)
            continue;
        DisjointSet sets(n);
        long long cost = 0;
        bool tree = true;
        for (std::size_t i = 0; i < edges.size() && tree; ++i)
            if (mask & (1u << static_cast<std::uint32_t>(i))) {
                tree = sets.unite(edges[i].u, edges[i].v);
                cost += edges[i].weight;
            }
        if (tree && (!best || cost < *best))
            best = cost;
    }
    return best;
}

int self_test()
{
    std::uint64_t s = 2026;
    auto below = [&s](int n) {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<int>((s >> 33) % static_cast<std::uint64_t>(n));
    };
    int agree = 0;
    const int trials = 3000;
    for (int t = 0; t < trials; ++t) {
        const int n = 1 + below(7);
        std::vector<Edge> edges(static_cast<std::size_t>(below(12)));
        for (Edge& e : edges)
            e = {below(n), below(n), below(21) - 10};
        const auto fast = kruskal(n, edges);
        const auto slow = brute_force(n, edges);
        const bool ok = fast.has_value() == slow.has_value() && (!fast || fast->cost == *slow);
        if (!ok)
            std::cout << "FAILED trial " << t << '\n';
        agree += ok;
    }
    std::cout << "random graphs where Kruskal matches the brute force: " << agree << " of " << trials << '\n';
    return agree == trials ? 0 : 1;
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "--test")
        return self_test();

    const std::string names = "ABCDEF";
    const std::vector<Edge> offices{
        {0, 1, 4}, {0, 2, 3}, {1, 2, 1}, {1, 3, 2}, {2, 3, 4},
        {3, 4, 2}, {4, 5, 6}, {3, 5, 7}, {2, 4, 5}, {1, 5, 9},
    };
    const auto tree = kruskal(6, offices);
    if (!tree) {
        std::cout << "the offices cannot all be connected\n";
        return 1;
    }
    std::cout << "Kruskal's network (cost " << tree->cost << "):\n";
    for (const auto& [u, v, weight] : tree->edges)
        std::cout << "  " << names[static_cast<std::size_t>(u)] << '-'
                  << names[static_cast<std::size_t>(v)] << (weight < 10 ? "  " : " ") << weight << '\n';
    return 0;
}
