/*
 * LAMGGTest.cpp
 *
 *  Created on: 20.11.2014
 *      Author: Michael
 */

#include <gtest/gtest.h>

#include <networkit/algebraic/CSRMatrix.hpp>
#include <networkit/algebraic/Vector.hpp>
#include <networkit/components/ConnectedComponents.hpp>
#include <networkit/graph/Graph.hpp>
#include <networkit/numerics/LAMG/Lamg.hpp>

namespace NetworKit {

// list of variants:
// - solver: solve, parallelSolve
// - setup: connected, not connected, not connected with partition
// - components: one component, multiple components
// - parallelism: single thread, multiple threads

class LAMGGTest
    // params: Solver, Setup, Components, Parallelism
    : public testing::TestWithParam<
          std::tuple<std::string, std::string, std::string, std::string>> {
protected:
    // returns a graph and a vector of RHSs
    std::tuple<Graph, std::vector<Vector>> testData() const;
};

INSTANTIATE_TEST_SUITE_P(
    LAMGGTest, LAMGGTest,
    testing::Combine(testing::Values("solve", "parallelSolve", "loaderSolve"),
                     testing::Values("connected", "disconnected", "disconnectedWithPartition"),
                     testing::Values("one", "two"), testing::Values("single", "four")));

// Temporary function
inline std::ostream &operator<<(std::ostream &os, const CSRMatrix &M) {
    for (index i = 0; i < M.numberOfRows(); i++) {
        for (index j = 0; j < M.numberOfColumns(); j++)
            os << M(i, j) << ", ";
        os << std::endl;
    }
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const Vector &vec) {
    os << "[";
    for (index i = 0; i < vec.getDimension(); i++) {
        if (i != 0)
            os << ", ";
        os << vec[i];
    }
    os << "]";
    return os;
}

inline bool vector_almost_equal(const Vector &lhs, const Vector &rhs) {
    if (lhs.getDimension() != rhs.getDimension())
        return false;
    for (size_t i = 0; i < lhs.getDimension(); ++i) {
        if (std::abs(lhs[i] - rhs[i]) > 1e-4)
            return false;
    }
    return true;
}

std::tuple<Graph, std::vector<Vector>> LAMGGTest::testData() const {
    auto [solveFn, setupFn, components, parallelism] = GetParam();

    Graph G(6);
    std::vector<Vector> rhss;
    std::vector<Vector> solutions;
    if (components == "one") {
        /*
        0 - 1 - 2
        |       |
        3 - 4 - 5

        L:
         2 -1  0 -1  0  0
        -1  2 -1  0  0  0
         0 -1  2  0  0 -1
        -1  0  0  2 -1  0
         0  0  0 -1  2 -1
         0  0 -1  0 -1  2
        */

        G.addEdge(0, 1);
        G.addEdge(0, 3);
        G.addEdge(1, 2);
        G.addEdge(2, 5);
        G.addEdge(3, 4);
        G.addEdge(4, 5);

        rhss = {
            {-4, 0, -2, 2, 0, 4},  {-8, 2, 2, 14, -8, -10}, {4, 0, -2, 2, 0, 4},
            {8, 2, 10, 0, -8, -1}, {4, 0, 6, 9, 0, 4},      {8, 8, -10, 5, -9, -3},
            {4, 0, 7, 2, 0, -4},   {8, 9, -7, 14, 8, -2},
        };

        // make sure that each vector sums up to 0
        for (auto &v : rhss) {
            double sum = 0;
            v.forElements([&](double value) { sum += value; });
            v[0] -= sum;
        }

        return {G, rhss};
    }

    if (components == "two") {
        /*
       0 - 1 - 2

       3 - 4 - 5

       L:
        1 -1  0  0  0  0
       -1  2 -1  0  0  0
        0 -1  1  0  0  0
        0  0  0  1 -1  0
        0  0  0 -1  2 -1
        0  0  0  0 -1  1
       */

        G.addEdge(0, 1);
        G.addEdge(1, 2);
        G.addEdge(3, 4);
        G.addEdge(4, 5);

        rhss = {
            {-1, 3, -2, -1, -3, 4},    {0, 0, 0, -1, -3, 4},       {-1, 3, -2, 0, 0, 0},
            {-10, 0, 10, 16, -6, -10}, {1, 3, 2, 1, 3, 4},         {0, 0, 0, -1, 3, 4},
            {1, 3, -2, 0, 0, 0},       {10, 0, -10, -16, -6, -10},
        };

        // make sure that each component sums up to 0
        for (auto &v : rhss) {
            v[0] -= v[0] + v[1] + v[2];
            v[3] -= v[3] + v[4] + v[5];
        }

        return {G, rhss};
    }
    throw std::logic_error("unknown number of components.");
}

TEST_P(LAMGGTest, testLamgVariants) {
    auto [solveFn, setupFn, components, parallelism] = GetParam();

    if (parallelism == "single")
        omp_set_num_threads(1);
    else if (parallelism == "four")
        omp_set_num_threads(4);
    else
        throw std::logic_error("unhandled variant!");

    // skip combinations that do not work
    if (setupFn == "connected" && components != "one")
        return;

    auto [G, rhss] = testData();

    auto L = CSRMatrix::laplacianMatrix(G);

    Lamg<CSRMatrix> lamg;

    ParallelConnectedComponents pcc(G);
    pcc.run();

    if (setupFn == "connected")
        lamg.setupConnected(L);
    else if (setupFn == "disconnected")
        lamg.setup(L);
    else if (setupFn == "disconnectedWithPartition")
        lamg.setup(L, G, pcc);
    else
        throw std::logic_error("unhandled variant!");

    // for loader solve variant
    const auto rhsLoader = [&rhss](count i, Vector &rhs) -> Vector & {
        rhs = rhss[i];
        return rhs;
    };

    count numProcessorCalls = 0;
    const auto resultProcessor = [&](count i, const Vector &result) {
        EXPECT_TRUE(vector_almost_equal(L * result, rhss[i]))
            << "Lamg result: " << result << "\nL * result: " << L * result << "\nrhs: " << rhss[i];
        ++numProcessorCalls;
    };

    if (solveFn == "solve") {
        for (size_t i = 0; i < rhss.size(); ++i) {
            const auto &rhs = rhss[i];

            Vector result(rhs.getDimension());

            lamg.solve(rhs, result);

            EXPECT_TRUE(vector_almost_equal(L * result, rhs))
                << "Lamg result: " << result << "\nL * result: " << L * result << "\nrhs: " << rhs;
        }
    } else if (solveFn == "parallelSolve") {
        std::vector<Vector> results(rhss.size(), Vector(6));
        lamg.parallelSolve(rhss, results);
        for (size_t i = 0; i < rhss.size(); ++i) {
            const auto &rhs = rhss[i];
            const auto &result = results[i];
            EXPECT_TRUE(vector_almost_equal(L * result, rhs))
                << "Lamg result: " << result << "\nL * result: " << L * result << "\nrhs: " << rhs;
        }
    } else if (solveFn == "loaderSolve") {
        lamg.parallelSolve(rhsLoader, resultProcessor, {rhss.size(), L.numberOfColumns()});
        EXPECT_EQ(numProcessorCalls, rhss.size());
    } else
        throw std::logic_error("unhandled variant!");
}

} /* namespace NetworKit */
