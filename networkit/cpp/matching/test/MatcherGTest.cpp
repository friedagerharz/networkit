/*
 * MatcherGTest.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: Henning
 */

#include <gtest/gtest.h>

#include <networkit/auxiliary/Random.hpp>
#include <networkit/graph/Graph.hpp>
#include <networkit/graph/GraphTools.hpp>
#include <networkit/io/DibapGraphReader.hpp>
#include <networkit/io/METISGraphReader.hpp>
#include <networkit/io/MTXGraphReader.hpp>
#include <networkit/matching/BMatcher.hpp>
#include <networkit/matching/BMatching.hpp>
#include <networkit/matching/BSuitorMatcher.hpp>
#include <networkit/matching/LocalMaxMatcher.hpp>
#include <networkit/matching/Matcher.hpp>
#include <networkit/matching/Matching.hpp>
#include <networkit/matching/PathGrowingMatcher.hpp>
#include <networkit/matching/SuitorMatcher.hpp>

#include <chrono>
#include <iostream>

namespace NetworKit {

// using std::chrono::duration;
// using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::seconds;
typedef std::chrono::duration<float, std::milli> dur;

class MatcherGTest : public testing::Test {
protected:
    // TODO use template instead of an overload when Matching base class is done
    bool hasUnmatchedNeighbors(const Graph &G, const BMatching &M) {
        for (const auto e : G.edgeRange())
            if (M.isUnmatched(e.u) && M.isUnmatched(e.v))
                return true;
        return false;
    }

    bool hasUnmatchedNeighbors(const Graph &G, const Matching &M) {
        for (const auto e : G.edgeRange())
            if (!M.isMatched(e.u) && !M.isMatched(e.v))
                return true;
        return false;
    }

    auto getTime() { return high_resolution_clock::now(); }

    void writeMatching(BMatching M, Graph G, std::string file_name) {
        auto matrix = M.getMatrix();
        std::ofstream fout;
        std::string path = file_name; //"matching_out/" + file_name + ".net";

        fout.open(path.c_str(), std::ios::out);
        if (fout.is_open()) {
            fout << "*Vertices " << G.numberOfNodes() << std::endl;
            fout << "*arcs" << std::endl;
            for (int i = 0; i < G.numberOfNodes(); i++)
                for (int j = 0; j < matrix[i].size(); j++)
                    if (i > matrix[i][j])
                        fout << i + 1 << " " << matrix[i][j] + 1 << " " << G.weight(i, matrix[i][j])
                             << std::endl;
        }
        fout.close();
    }
};

TEST_F(MatcherGTest, testLocalMaxMatching) {
    {
        Graph G(10, true, true);
        EXPECT_THROW(LocalMaxMatcher{G}, std::runtime_error);
    }

    count n = 50;
    Graph G(n);
    G.forNodePairs([&](node u, node v) { G.addEdge(u, v); });

    LocalMaxMatcher localMaxMatcher(G);

    TRACE("Start localMax matching");
    localMaxMatcher.run();
    Matching M = localMaxMatcher.getMatching();
    TRACE("Finished localMax matching");

    count numExpEdges = n / 2;
    bool isProper = M.isProper(G);
    EXPECT_TRUE(isProper);
    EXPECT_EQ(M.size(G), numExpEdges);

#if !defined _WIN32 && !defined _WIN64 && !defined WIN32 && !defined WIN64
    DibapGraphReader reader;
    Graph airfoil1 = reader.read("input/airfoil1.gi");
    LocalMaxMatcher lmm(airfoil1);
    lmm.run();
    M = lmm.getMatching();
    isProper = M.isProper(airfoil1);
    EXPECT_TRUE(isProper);
    DEBUG("LocalMax on airfoil1 produces matching of size: ", M.size(G));
#endif
}

TEST_F(MatcherGTest, testLocalMaxMatchingDirectedWarning) {
    Graph G(2, false, true);
    G.addEdge(0, 1);
    EXPECT_THROW(LocalMaxMatcher localMaxMatcher(G), std::runtime_error);
}

TEST_F(MatcherGTest, testPgaMatchingOnWeightedGraph) {
    count n = 50;
    Graph G(n);
    G.forNodePairs([&](node u, node v) { G.addEdge(u, v, Aux::Random::real()); });
    PathGrowingMatcher pgaMatcher(G);
    EXPECT_NO_THROW(pgaMatcher.run());
}

TEST_F(MatcherGTest, testPgaMatchingWithSelfLoops) {
    count n = 50;
    Graph G(n);
    G.forNodePairs([&](node u, node v) { G.addEdge(u, v, Aux::Random::real()); });
    G.forNodes([&](node u) { G.addEdge(u, u); });
    EXPECT_THROW(PathGrowingMatcher pgaMatcher(G), std::invalid_argument);
}

TEST_F(MatcherGTest, testPgaMatching) {
    count n = 50;
    Graph G(n);
    G.forNodePairs([&](node u, node v) { G.addEdge(u, v); });
    PathGrowingMatcher pgaMatcher(G);

    DEBUG("Start PGA matching on 50-clique");

    pgaMatcher.run();
    Matching M = pgaMatcher.getMatching();

    count numExpEdges = n / 2;
    bool isProper = M.isProper(G);
    EXPECT_TRUE(isProper);
    EXPECT_EQ(M.size(G), numExpEdges);
    DEBUG("Finished PGA matching on 50-clique");

#if !defined _WIN32 && !defined _WIN64 && !defined WIN32 && !defined WIN64
    DibapGraphReader reader;
    Graph airfoil1 = reader.read("input/airfoil1.gi");
    PathGrowingMatcher pga2(airfoil1);
    pga2.run();
    M = pga2.getMatching();
    isProper = M.isProper(airfoil1);
    EXPECT_TRUE(isProper);
    DEBUG("PGA on airfoil1 produces matching of size: ", M.size(G));
#endif
}

TEST_F(MatcherGTest, debugValidMatching) {
    METISGraphReader reader;
    Graph G = reader.read("coAuthorsDBLP.graph");

    LocalMaxMatcher pmatcher(G);
    pmatcher.run();
    Matching M = pmatcher.getMatching();

    bool isProper = M.isProper(G);
    EXPECT_TRUE(isProper);
}

TEST_F(MatcherGTest, testSuitorMatcher) {
    { // Directed graphs are not supported
        Graph G(10, true, true);
        EXPECT_THROW(SuitorMatcher{G}, std::runtime_error);
    }

    { // Graphs with self loops are not supported
        Graph G(10);
        G.addEdge(0, 0);
        G.addEdge(0, 0);
        EXPECT_THROW(SuitorMatcher{G}, std::runtime_error);
    }

    const edgeweight maxWeight = 10;

    const auto doTest = [&, maxWeight](Graph &G) -> void {
        // Test suitor matcher
        SuitorMatcher sm(G, false, true);
        sm.run();
        const auto M1 = sm.getMatching();
        EXPECT_TRUE(M1.isProper(G));
        EXPECT_FALSE(hasUnmatchedNeighbors(G, M1));

        GraphTools::sortEdgesByWeight(G, true);
        {
            auto G1 = G;
            G1.addEdge(0, 1, maxWeight);
            G1.addEdge(0, 2, maxWeight);
            EXPECT_THROW(SuitorMatcher(G1, true, true), std::runtime_error);
        }

        // Test sort suitor matcher
        SuitorMatcher ssm(G, true, true);
        ssm.run();
        const auto M2 = ssm.getMatching();
        EXPECT_TRUE(M2.isProper(G));
        EXPECT_FALSE(hasUnmatchedNeighbors(G, M2));

        // Matchings must be the same
        G.forNodes([&M1, &M2](node u) { EXPECT_EQ(M1.mate(u), M2.mate(u)); });
    };

    for (int seed : {1, 2, 3}) {
        Aux::Random::setSeed(seed, true);

        // Test unweighted
        auto G = METISGraphReader{}.read("input/PGPgiantcompo.graph");
        G.removeSelfLoops();
        G.removeMultiEdges();
        doTest(G);

        // Test weighted
        G = GraphTools::toWeighted(G);
        G.forEdges(
            [&G, maxWeight](node u, node v) { G.setWeight(u, v, Aux::Random::real(maxWeight)); });
        doTest(G);
    }
}

TEST_F(MatcherGTest, REMOVE) {
    auto start_time = getTime();
    auto G = MTXGraphReader{}.read("input/large_graph/astro-ph.mtx");
    G.removeSelfLoops();
    G.removeMultiEdges();

    auto reading_time = getTime();
    dur t1 = high_resolution_clock::now() - reading_time;
    std::cout << "Initialization Time: " << t1.count() << "s\n";

    // Test suitor matcher
    BSuitorMatcher sm(G, 2);
    sm.run();

    auto matching_time = getTime();
    dur t2 = matching_time - reading_time;
    dur t3 = matching_time - start_time;
    std::cout << "Matching Time: " << t2.count() << "s\n";
    std::cout << "Total Time: " << t3.count() << "s\n";

    const auto M1 = sm.getBMatching();
    EXPECT_TRUE(M1.isProper(G));
    EXPECT_FALSE(hasUnmatchedNeighbors(G, M1));

    std::cout << "Matching verified.\n";
    auto before_weight_time = getTime();

    std::cout << "Matching Weight: " << M1.weight(G) << std::endl;
    auto weight_time = getTime();
    dur t4 = weight_time - before_weight_time;
    std::cout << "in : " << t4.count() << "s\n";
}

TEST_F(MatcherGTest, testBSuitorMatcherInvalidGraphDirected) {
    Graph G(10, true, true);
    EXPECT_THROW(BSuitorMatcher(G, 2), std::runtime_error);
}

TEST_F(MatcherGTest, testBSuitorMatcherInvalidGraphSelfLoops) {
    Graph G(10);
    G.addEdge(0, 0);
    G.addEdge(0, 0);
    EXPECT_THROW(BSuitorMatcher(G, 2), std::runtime_error);
}

TEST_F(MatcherGTest, testBSuitorMatcherTieBreaking) {
    auto G = METISGraphReader{}.read("input/tie.graph");
    G.removeSelfLoops();
    G.removeMultiEdges();

    BSuitorMatcher bsm(G, 4);
    bsm.run();
    const auto M = bsm.getBMatching();

    EXPECT_TRUE(M.isProper(G));
    EXPECT_FALSE(hasUnmatchedNeighbors(G, M));

    writeMatching(M, G, "tie");
}

TEST_F(MatcherGTest, testBSuitorMatcherEqualsSuitorMatcher) {
    auto G = METISGraphReader{}.read("input/lesmis.graph");
    G.removeSelfLoops();
    G.removeMultiEdges();

    SuitorMatcher sm(G, false, false);
    sm.run();
    const auto M = sm.getMatching();

    BSuitorMatcher bsm(G, 1);
    bsm.run();
    const auto bM = bsm.getBMatching();

    EXPECT_TRUE(bM.isProper(G));
    EXPECT_TRUE(M.isProper(G));
    EXPECT_FALSE(hasUnmatchedNeighbors(G, M));
    EXPECT_FALSE(hasUnmatchedNeighbors(G, bM));
}

TEST_F(MatcherGTest, testBSuitorMatcherConstantB) {
    for (int b : {2, 3, 4, 5}) {
        auto G = METISGraphReader{}.read("input/lesmis.graph");
        G.removeSelfLoops();
        G.removeMultiEdges();
        BSuitorMatcher bsm(G, b);
        bsm.run();
        const auto M = bsm.getBMatching();
        EXPECT_TRUE(M.isProper(G));
        EXPECT_FALSE(hasUnmatchedNeighbors(G, M));
    }
}

TEST_F(MatcherGTest, testBSuitorMatcherDifferentB) {
    Aux::Random::setSeed(1, true);

    auto G = METISGraphReader{}.read("input/lesmis.graph");
    G.removeSelfLoops();
    G.removeMultiEdges();
    std::vector<count> b;
    for (count i = 0; i < G.numberOfNodes(); i++) {
        b.emplace_back(Aux::Random::integer(1, (G.numberOfNodes() - 1)));
    }

    BSuitorMatcher bsm(G, b);
    bsm.run();
    const auto M = bsm.getBMatching();
    EXPECT_TRUE(M.isProper(G));
    EXPECT_FALSE(hasUnmatchedNeighbors(G, M));
}

// TEST_F(MatcherGTest, testBSuitorMatcherExecutionTimeMeasurement) {
//     auto start_time = getTime();
//     auto G = MTXGraphReader{}.read("input/astro-ph.mtx");
//     G.removeSelfLoops();
//     G.removeMultiEdges();
//     auto reading_time = getTime();
//     std::cout << "Initialization Time: " << reading_time - start_time << "s\n";

//     const int b = 2;
//     BSuitorMatcher bsm(G, b);
//     bsm.run();
//     auto matching_time = getTime();
//     std::cout << "Matching Time: " << matching_time - reading_time << "s\n";
//     std::cout << "Total Time: " << matching_time - start_time << "s\n";

//     const auto M = bsm.getBMatching();
//     EXPECT_TRUE(M.isProper(G));
//     std::cout << "Matching verified.\n";

//     std::cout << "Matching Weight: " << M.weight(G) << std::endl;

//     // Matching output:

//     // auto matrix = M.getMatrix();
//     // std::ofstream fout;
//     // fout.open("matching.net", std::ios::out);
//     // if (fout.is_open()) {
//     //     fout << "*Vertices " << G.numberOfNodes() << std::endl;
//     //     fout << "*arcs" << std::endl;
//     //     for (int i = 0; i < G.numberOfNodes(); i++)
//     //         for (int j = 0; j < matrix[i].size(); j++)
//     //             if (i > matrix[i][j])
//     //                 fout << i + 1 << " " << matrix[i][j] + 1 << " " << G.weight(i,
//     matrix[i][j])
//     //                      << std::endl;
//     // }
//     // fout.close();
// }

} // namespace NetworKit
