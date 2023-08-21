#include <networkit/auxiliary/Enforce.hpp>
#include <networkit/auxiliary/Log.hpp>
#include <networkit/auxiliary/StringTools.hpp>
#include <networkit/io/MTXGraphReader.hpp>
#include <networkit/io/MTXParser.hpp>

namespace NetworKit {

Graph MTXGraphReader::read(const std::string &path) {
    MTXParser parser(path);

    auto header = parser.getHeader();
    auto size = parser.getMatrixSize();
    auto weighted = true;

    if (header.field == MTXParser::Field::Pattern)
        weighted = false;

    Graph G(std::max(size.rows, size.columns), weighted);
    std::string graphName =
        Aux::StringTools::split(Aux::StringTools::split(path, '/').back(), '.').front();

    while (parser.hasNext()) {
        auto edge = parser.getNext(weighted);
        auto from = std::get<0>(edge);
        auto to = std::get<1>(edge);
        auto weight = std::get<2>(edge);
        weight.has_value() ? G.addPartialEdge(unsafe, from, to, *weight)
                           : G.addPartialEdge(unsafe, from, to);
    }
    return G;
}

} /* namespace NetworKit */
