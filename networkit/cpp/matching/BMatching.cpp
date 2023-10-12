#include <networkit/matching/BMatching.hpp>

namespace NetworKit {

BMatching::BMatching(const std::vector<count> &b, count z) : b(b) {
    data.reserve(z);
    for (count i = 0; i < z; ++i)
        data.emplace_back(b.at(i), none);
}

bool BMatching::isProper(const Graph &G) const {
    // check if entries are symmetric
    for (node v : G.nodeRange()) {
        for (index i = 0; i < b.at(v); i++) {
            node w = data[v][i];
            if (w != none && std::find(data[w].begin(), data[w].end(), v) == data[w].end()) {
                DEBUG("node ", v, " is not symmetrically matched");
                return false;
            }
        }
    }

    // check if every pair exists as an edge
    for (node v : G.nodeRange()) {
        for (index i = 0; i < b.at(v); i++) {
            node w = data[v][i];
            if ((v != w) && (w != none) && !G.hasEdge(v, w)) {
                DEBUG("matched pair (", v, ",", w, ") is not an edge");
                return false;
            }
        }
    }

    return true;
}

void BMatching::match(node u, node v) {
    {
        auto i = findFirstFreeIndex(u);
        assert(i != none);
        data[u][i] = v;
    }
    {
        auto i = findFirstFreeIndex(v);
        assert(i != none);
        data[v][i] = u;
    }
}

void BMatching::unmatch(node u, node v) {
    {
        auto i = findIndexOf(u, v);
        assert(i != none);
        data[u][i] = none;
    }
    {
        auto i = findIndexOf(v, u);
        assert(i != none);
        data[v][i] = none;
    }
}

bool BMatching::isUnmatched(node u) const {
    return (data[u].front() == none);
}

bool BMatching::areMatched(node u, node v) const {
    return std::find(data[u].begin(), data[u].end(), v) != data[u].end();
}

count BMatching::size(const Graph &G) const {
    count size = 0;
    G.forNodes([&](node v) {
        if (!isUnmatched(v)) {
            ++size;
        }
    });
    return size / 2;
}

std::vector<node> BMatching::mates(node v) const {
    return data[v];
}

edgeweight BMatching::weight(const Graph &G) const {
    return G.parallelSumForNodes([&](node v) {
        edgeweight weight_per_node = 0.0;
        for (auto u : mates(v)) {
            if (v < u) {
                weight_per_node += G.weight(v, u);
            }
        }
        return weight_per_node;
    });
}

const std::vector<std::vector<node>> &BMatching::getMatrix() const {
    return data;
}

std::vector<count> BMatching::getB() const {
    return b;
}

index BMatching::findFirstFreeIndex(node u) const {
    return findIndexOf(u, none);
}

index BMatching::findIndexOf(node u, node x) const {
    auto i = std::find(data[u].begin(), data[u].end(), x);
    return (i == std::end(data[u])) ? none : i - data[u].begin();
}

} // namespace NetworKit