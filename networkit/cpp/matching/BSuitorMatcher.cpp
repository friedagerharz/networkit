#include <networkit/matching/BSuitorMatcher.hpp>

#include <algorithm>
#include <stdexcept>

namespace NetworKit {
BSuitorMatcher::BSuitorMatcher(const Graph &G, const std::vector<count> &b) : BMatcher(G, b), b(b) {
    if (G.numberOfSelfLoops() > 0)
        throw std::runtime_error("This algorithm does not support graphs with self-loops.");

    if (G.isDirected())
        throw std::runtime_error("This algorithm does not support directed graphs.");

    if (b.size() != G.numberOfNodes())
        throw std::runtime_error(
            "The number of b values does not match the number of nodes in this graph.");
}

BSuitorMatcher::BSuitorMatcher(const Graph &G, count b)
    : BSuitorMatcher(G, std::vector<count>(G.numberOfNodes(), b)) {}

BSuitorMatcher::BSuitorMatcher(const Graph &G, const std::string &path)
    : BSuitorMatcher(G, readBValuesFromFile(G.numberOfNodes(), path)) {}

void BSuitorMatcher::findSuitors(node u) {
    for (count i = 0; i < b.at(u); i++) {
        auto x = findPreferred(u);
        if (x != none) {
            // auto t1 = high_resolution_clock::now();
            makeSuitor(u, x);
            // auto t2 = high_resolution_clock::now();
            // std::cout << "              (BSuitorMatcher::findSuitors) done after "
            //           << duration_cast<seconds>(t2 - t1).count() << "s\n";
        }
    }
}

node BSuitorMatcher::findPreferred(node y) {
    // auto t1 = high_resolution_clock::now();

    node x = none;
    edgeweight heaviest = 0;

    auto hasProposedTo = [&](node n) -> bool {
        return (std::find(proposed[y].begin(), proposed[y].end(), n) != proposed[y].end());
    };

    for (auto v : G->weightNeighborRange(y)) {
        if (!hasProposedTo(v.first)) {
            const edgeweight weight = v.second;
            if (weight > heaviest || (weight == heaviest && v.first < x)) {
                if (weight > G->weight(v.first, suitors[v.first].back())
                    || (weight == G->weight(v.first, suitors[v.first].back())
                        && y < suitors[v.first].back())) {
                    x = v.first;
                    heaviest = weight;
                }
            }
        }
    }
    // auto t2 = high_resolution_clock::now();
    // std::cout << "      (BSuitorMatcher::findPreferred) done after "
    //           << duration_cast<seconds>(t2 - t1).count() << "s\n";

    return x;
}

void BSuitorMatcher::makeSuitor(node u, node x) {

    auto y = suitors[x].back();
    sortInsert(suitors[x], x, u);
    auto i = findFirstFreeIndex(proposed[u]);
    proposed[u][i] = x;

    if (y != none) {
        sortRemove(proposed[y], x);
        auto z = findPreferred(y);

        if (z != none) {
            makeSuitor(y, z);
        }
    }
}

std::vector<count> BSuitorMatcher::readBValuesFromFile(count size, const std::string &path) const {
    std::vector<count> b;
    b.reserve(size);
    std::ifstream file(path);
    std::string line;
    int line_number = 1;

    while (std::getline(file, line)) {
        std::istringstream istring(line);
        int val;
        if (!(istring >> val)) {
            throw std::runtime_error("File " + path + " contains an invalid value in line "
                                     + std::to_string(line_number) + ".");
        }
        if (istring >> val) {
            throw std::runtime_error("File " + path + " contains multiple values in line "
                                     + std::to_string(line_number) + ".");
        }
        if (val < 0) {
            throw std::runtime_error("File " + path + " contains a negative value in line "
                                     + std::to_string(line_number) + ".");
        }
        b.push_back(val);
        line_number++;
    }
    if (b.size() != size) {
        throw std::runtime_error("The number of values in file " + path
                                 + " does not match the number of nodes in this graph.");
    }
    return b;
}

void BSuitorMatcher::sortInsert(std::vector<node> &nodes, node u, node v) {
    auto i = std::find_if(nodes.begin(), nodes.end(), [&](const node &y) {
        return (G->weight(u, v) > G->weight(u, y))
               || ((G->weight(u, v) == G->weight(u, y)) && (v < y));
    });
    nodes.insert(i, v);
    nodes.pop_back();
}

void BSuitorMatcher::sortRemove(std::vector<node> &nodes, const node u) {
    nodes.erase(std::remove(nodes.begin(), nodes.end(), u), nodes.end());
    nodes.push_back(none);
}

index BSuitorMatcher::findFirstFreeIndex(const std::vector<node> &nodes) const {
    return findIndexOf(nodes, none);
}

index BSuitorMatcher::findIndexOf(const std::vector<node> &nodes, node x) const {
    auto i = std::find(nodes.begin(), nodes.end(), x);
    return i == nodes.end() ? none : i - nodes.begin();
}

void BSuitorMatcher::checkSymmetry() const {
    auto areMatchedSymmetrical = [&](node u, node v) -> void {
        [[maybe_unused]] auto i_1 =
            std::find(suitors[u].begin(), suitors[u].end(), v) != suitors[u].end();
        [[maybe_unused]] auto i_2 =
            std::find(suitors[v].begin(), suitors[v].end(), u) != suitors[v].end();
        assert(i_1 == i_2);
    };

    G->forNodes([&](node u) { G->forNodes([&](node v) { areMatchedSymmetrical(u, v); }); });
}

void BSuitorMatcher::run() {

    // auto t1 = high_resolution_clock::now();

    const auto n = G->upperNodeIdBound();
    for (index i = 0; i < n; i++) {
        auto v = std::vector<node>(b.at(i), none);
        suitors.emplace_back(v);
        proposed.emplace_back(v);
    }

    // auto t2 = high_resolution_clock::now();
    // std::cout << "(BSuitorMatcher::run) Initialization done after "
    //           << duration_cast<seconds>(t2 - t1).count() << "s\n";

    G->forNodes([&](node u) { findSuitors(u); });

    // auto t3 = high_resolution_clock::now();
    // std::cout << "(BSuitorMatcher::run) FindSuitors done after "
    //           << duration_cast<seconds>(t3 - t2).count() << "s\n";

    checkSymmetry();

    // auto t4 = high_resolution_clock::now();
    // std::cout << "(BSuitorMatcher::run) CheckSymmetry done after "
    //           << duration_cast<seconds>(t4 - t3).count() << "s\n";

    // TODO make parallel
    G->forNodes([&suitors = suitors, &M = M](node u) {
        for (auto p : suitors[u]) {
            if (p != none && u < p) { // Ensure we match a pair of nodes only once
                M.match(u, p);
            }
        }
    });

    // auto t5 = high_resolution_clock::now();
    // std::cout << "(BSuitorMatcher::run) Insertion into matching done after "
    //           << duration_cast<seconds>(t5 - t4).count() << "s\n";

    hasRun = true;
}
} // namespace NetworKit