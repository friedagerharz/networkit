/*
 * Matching.hpp
 *
 *  Created on: 03.12.2012
 */

#ifndef NETWORKIT_MATCHING_MATCHING_HPP_
#define NETWORKIT_MATCHING_MATCHING_HPP_

#include <networkit/auxiliary/Log.hpp>
#include <networkit/graph/Graph.hpp>
#include <networkit/matching/IMatching.hpp>
#include <networkit/structures/Partition.hpp>

namespace NetworKit {

/**
 * @ingroup matching
 */
class Matching : public IMatching {

public:
    /**
     * Construct new Matching.
     *
     * @param[in]  z  Maximum number of nodes.
     */
    Matching(count z = 0);

    /** Default destructor */
    ~Matching() override = default;

    /**
     * Check if node is matched.
     *
     * @param[in]  u   node.
     * @return @c true if u is matched.
     */
    bool isMatched(node u) const;

    /**
     * Get the matched neighbor of @a v if it exists, otherwise @c none.
     *
     * @param[in] v node.
     * @return Mate of @a v if it exists, otherwise none.
     */
    index mate(node v) const;

    /**
     * Convert matching to a Partition object where matched nodes
     * belong to the same subset and unmatched nodes belong to a singleton subset.
     * @return Partition
     */
    Partition toPartition(const Graph &G) const;

    /**
     * Get the actual vector storing the data.
     * @return vector
     */
    std::vector<node> getVector() const;
};

} /* namespace NetworKit */
#endif // NETWORKIT_MATCHING_MATCHING_HPP_
