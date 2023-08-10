/*
 * Matching.cpp
 *
 *  Created on: 03.12.2012
 */

#include <networkit/matching/Matching.hpp>

namespace NetworKit {

Matching::Matching(count z) : IMatching(z) {}

bool Matching::isMatched(node u) const {
    return (this->data.at(u).front() != none);
}

index Matching::mate(node v) const {
    const auto mate = mates(v);
    assert(mate.size() == 1);
    return mate.front();
}

Partition Matching::toPartition(const Graph &G) const {
    Partition partition(G.upperNodeIdBound());
    std::vector<bool> visited(G.upperNodeIdBound(), false);
    G.forNodes([&](node u) {
        if (!visited[u]) {
            if (mate(u) == none) {
                partition.addToSubset(u, u);
            } else {
                partition.addToSubset(u, u);
                partition.addToSubset(u, mate(u));
                visited[u] = true;
                visited[mate(u)] = true;
            }
        }
    });
    return partition;
}

std::vector<node> Matching::getVector() const {
    const auto matrix = getMatrix();
    std::vector<node> vec(matrix.size(), none);
    for (int i = 0; i < matrix.size(); i++) {
        assert(matrix.at(i).size() == 1);
        vec.at(i) = matrix.at(i).front();
    }
    return vec;
}

} // namespace NetworKit
  /* namespace NetworKit */
