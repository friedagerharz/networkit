#ifndef NETWORKIT_MATCHING_B_MATCHING_HPP_
#define NETWORKIT_MATCHING_B_MATCHING_HPP_

#include <networkit/auxiliary/Log.hpp>
#include <networkit/graph/Graph.hpp>
#include <networkit/matching/IMatching.hpp>

namespace NetworKit {

/**
 * @ingroup b-matching
 */
class BMatching : public IMatching {

public:
    /**
     * Constructs a new BMatching.
     *
     * @param z Maximum number of nodes.
     * @param b b value
     */
    BMatching(count z = 0, count b = 1);

    /** Default destructor */
    ~BMatching() override = default;
};

} /* namespace NetworKit */
#endif // NETWORKIT_MATCHING_B_MATCHING_HPP_
