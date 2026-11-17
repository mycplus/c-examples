// route.cpp: the same route planner in C++17, using the standard library's
// containers and priority queue. Same arguments, output and exit codes as
// the C version:
//   route_cpp MAP FROM TO [dijkstra|floyd]
//   route_cpp MAP --from TOWN
//   route_cpp MAP --table
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Dist = long long;
constexpr Dist kInf = std::numeric_limits<Dist>::max();

struct Edge {
    int to;
    Dist weight;
};

class Graph {
public:
    int add_node(const std::string& name)
    {
        auto [it, inserted] = index_.try_emplace(name, static_cast<int>(names_.size()));
        if (inserted) {
            names_.push_back(name);
            adj_.emplace_back();
        }
        return it->second;
    }
    std::optional<int> find(const std::string& name) const
    {
        auto it = index_.find(name);
        if (it == index_.end())
            return std::nullopt;
        return it->second;
    }
    void add_road(int a, int b, Dist km)
    {
        adj_[a].push_back({b, km});
        adj_[b].push_back({a, km});
    }
    int size() const { return static_cast<int>(names_.size()); }
    const std::string& name(int i) const { return names_[i]; }
    const std::vector<Edge>& edges(int u) const { return adj_[u]; }

private:
    std::vector<std::string> names_;
    std::vector<std::vector<Edge>> adj_;
    std::unordered_map<std::string, int> index_;
};

std::string trim(const std::string& s)
{
    const char* space = " \t\r\n\f\v";
    auto first = s.find_first_not_of(space);
    if (first == std::string::npos)
        return "";
    return s.substr(first, s.find_last_not_of(space) - first + 1);
}

// Reads "Town A,Town B,distance" lines; throws with the line number on bad input.
Graph load_map(std::istream& in)
{
    Graph g;
    std::string line;
    for (int line_no = 1; std::getline(in, line); ++line_no) {
        std::string text = trim(line);
        if (text.empty() || text[0] == '#')
            continue;
        if (std::count(text.begin(), text.end(), ',') != 2)
            throw std::runtime_error("line " + std::to_string(line_no) + ": expected \"town,town,distance\"");
        std::istringstream fields(text);
        std::string a, b, d;
        std::getline(fields, a, ',');
        std::getline(fields, b, ',');
        std::getline(fields, d);
        a = trim(a);
        b = trim(b);
        d = trim(d);
        std::size_t used = 0;
        Dist km = 0;
        try {
            km = std::stoll(d, &used);
        } catch (const std::exception&) {
            used = 0;
        }
        if (a.empty() || b.empty() || d.empty() || used != d.size() || km <= 0 || km > 1000000000LL)
            throw std::runtime_error("line " + std::to_string(line_no) +
                                     ": needs two town names and a distance from 1 to 1000000000");
        if (a == b)
            throw std::runtime_error("line " + std::to_string(line_no) + ": a road from \"" + a + "\" to itself");
        // Two statements, not g.add_road(g.add_node(a), g.add_node(b), km): the order in
        // which function arguments are evaluated is unspecified, and the node numbering
        // (and so the output order) would then depend on the compiler.
        const int u = g.add_node(a);
        const int v = g.add_node(b);
        g.add_road(u, v, km);
    }
    if (g.size() == 0)
        throw std::runtime_error("the map has no roads");
    return g;
}

struct SingleSource {
    std::vector<Dist> dist;
    std::vector<int> prev;
};

SingleSource dijkstra(const Graph& g, int source)
{
    SingleSource r{std::vector<Dist>(g.size(), kInf), std::vector<int>(g.size(), -1)};
    using Item = std::pair<Dist, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<>> queue;
    r.dist[source] = 0;
    queue.push({0, source});
    while (!queue.empty()) {
        auto [d, u] = queue.top();
        queue.pop();
        if (d > r.dist[u])
            continue;  // a stale entry
        for (const Edge& e : g.edges(u)) {
            if (d + e.weight < r.dist[e.to]) {
                r.dist[e.to] = d + e.weight;
                r.prev[e.to] = u;
                queue.push({r.dist[e.to], e.to});
            }
        }
    }
    return r;
}

struct AllPairs {
    int n;
    std::vector<Dist> dist;  // n * n, row-major
    std::vector<int> next;
    Dist at(int i, int j) const { return dist[static_cast<std::size_t>(i) * n + j]; }
};

AllPairs floyd_warshall(const Graph& g)
{
    const int n = g.size();
    const auto cell = [n](int i, int j) { return static_cast<std::size_t>(i) * n + j; };
    AllPairs r{n, std::vector<Dist>(cell(n, 0), kInf), std::vector<int>(cell(n, 0), -1)};
    for (int i = 0; i < n; ++i) {
        r.dist[cell(i, i)] = 0;
        r.next[cell(i, i)] = i;
        for (const Edge& e : g.edges(i))
            if (e.weight < r.dist[cell(i, e.to)]) {
                r.dist[cell(i, e.to)] = e.weight;
                r.next[cell(i, e.to)] = e.to;
            }
    }
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i) {
            if (r.dist[cell(i, k)] == kInf)
                continue;
            for (int j = 0; j < n; ++j) {
                if (r.dist[cell(k, j)] == kInf)
                    continue;
                Dist through_k = r.dist[cell(i, k)] + r.dist[cell(k, j)];
                if (through_k < r.dist[cell(i, j)]) {
                    r.dist[cell(i, j)] = through_k;
                    r.next[cell(i, j)] = r.next[cell(i, k)];
                }
            }
        }
    return r;  // the map loader allows positive lengths only, so no negative cycle is possible
}

std::vector<int> path_from_prev(const std::vector<int>& prev, int source, int target)
{
    std::vector<int> path;
    for (int v = target; v != -1; v = prev[v])
        path.push_back(v);
    if (path.back() != source)
        return {};
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> path_from_next(const AllPairs& ap, int source, int target)
{
    std::vector<int> path;
    if (ap.next[static_cast<std::size_t>(source) * ap.n + target] == -1)
        return path;
    for (int v = source; v != target; v = ap.next[static_cast<std::size_t>(v) * ap.n + target])
        path.push_back(v);
    path.push_back(target);
    return path;
}

int name_width(const Graph& g)
{
    std::size_t w = 4;
    for (int i = 0; i < g.size(); ++i)
        w = std::max(w, g.name(i).size());
    return static_cast<int>(w);
}

void print_route(const Graph& g, const std::vector<int>& path, const char* method)
{
    const int w = name_width(g);
    std::cout << g.name(path.front()) << " to " << g.name(path.back()) << " (" << method << ")\n\n";
    std::cout << std::left << "  " << std::setw(w) << "From" << "  " << std::setw(w) << "To" << std::right
              << "  " << std::setw(7) << "Road km" << "  " << std::setw(8) << "Total km" << '\n';
    Dist total = 0;
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        Dist road = kInf;
        for (const Edge& e : g.edges(path[i]))
            if (e.to == path[i + 1])
                road = std::min(road, e.weight);
        total += road;
        std::cout << std::left << "  " << std::setw(w) << g.name(path[i]) << "  " << std::setw(w)
                  << g.name(path[i + 1]) << std::right << "  " << std::setw(7) << road << "  " << std::setw(8)
                  << total << '\n';
    }
    const std::size_t roads = path.size() - 1;
    std::cout << "\nTotal: " << total << " km over " << roads << (roads == 1 ? " road" : " roads") << '\n';
}

std::optional<int> find_town(const Graph& g, const std::string& name)
{
    auto i = g.find(name);
    if (!i)
        std::cerr << "route: unknown town \"" << name << "\"\n";
    return i;
}

int one_route(const Graph& g, const std::string& from, const std::string& to, const std::string& method)
{
    auto s = find_town(g, from), t = find_town(g, to);
    if (!s || !t)
        return 2;
    if (method != "dijkstra" && method != "floyd") {
        std::cerr << "route: method must be dijkstra or floyd\n";
        return 2;
    }
    const bool use_floyd = method == "floyd";
    std::vector<int> path = use_floyd ? path_from_next(floyd_warshall(g), *s, *t)
                                      : path_from_prev(dijkstra(g, *s).prev, *s, *t);
    if (path.empty()) {
        std::cout << "No route from " << from << " to " << to << ": no chain of roads connects them.\n";
        return 1;
    }
    print_route(g, path, use_floyd ? "Floyd-Warshall" : "Dijkstra");
    return 0;
}

int from_town(const Graph& g, const std::string& from)
{
    auto s = find_town(g, from);
    if (!s)
        return 2;
    const int w = name_width(g);
    SingleSource r = dijkstra(g, *s);
    std::cout << "Distances from " << g.name(*s) << " (Dijkstra)\n\n";
    for (int v = 0; v < g.size(); ++v) {
        std::cout << "  " << std::left << std::setw(w) << g.name(v) << std::right << "  ";
        if (r.dist[v] == kInf)
            std::cout << "no route\n";
        else
            std::cout << std::setw(5) << r.dist[v] << " km  via " << (r.prev[v] < 0 ? "-" : g.name(r.prev[v]))
                      << '\n';
    }
    return 0;
}

int table(const Graph& g)
{
    AllPairs ap = floyd_warshall(g);
    std::cout << std::left << std::setw(6) << "km" << std::right;
    for (int j = 0; j < g.size(); ++j)
        std::cout << std::setw(6) << g.name(j).substr(0, 4);
    std::cout << '\n';
    for (int i = 0; i < g.size(); ++i) {
        std::cout << std::left << std::setw(6) << g.name(i).substr(0, 4) << std::right;
        for (int j = 0; j < g.size(); ++j) {
            if (ap.at(i, j) == kInf)
                std::cout << std::setw(6) << "-";
            else
                std::cout << std::setw(6) << ap.at(i, j);
        }
        std::cout << '\n';
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 5) {
        std::cerr << "usage: route MAP FROM TO [dijkstra|floyd]\n"
                     "       route MAP --from TOWN\n"
                     "       route MAP --table\n";
        return 2;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "route: cannot open " << argv[1] << '\n';
        return 2;
    }
    Graph g;
    try {
        g = load_map(in);
    } catch (const std::exception& e) {
        std::cerr << "route: " << argv[1] << ": " << e.what() << '\n';
        return 2;
    }

    const std::string mode = argv[2];
    if (mode == "--table" && argc == 3)
        return table(g);
    if (mode == "--from" && argc == 4)
        return from_town(g, argv[3]);
    if (argc >= 4 && mode[0] != '-')
        return one_route(g, argv[2], argv[3], argc == 5 ? argv[4] : "dijkstra");
    std::cerr << "route: bad arguments (run with no arguments for usage)\n";
    return 2;
}
