#include <networkit/auxiliary/Enforce.hpp>
#include <networkit/auxiliary/Log.hpp>
#include <networkit/auxiliary/NumberParsing.hpp>
#include <networkit/io/MTXParser.hpp>

#include <stdexcept>

namespace NetworKit {

/**
 * Extract a (weighted) edge from a line in the file.
 */
MTXParser::Edge parseLine(const std::string &line, const bool weighted) {
    node i, j;
    std::optional<double> value;
    std::istringstream istring(line);
    istring >> i >> j;
    i--;
    j--;

    if (weighted) {
        double tmp;
        if (istring >> tmp)
            value = tmp;
    }
    return MTXParser::Edge(i, j, value);
}

MTXParser::MTXParser(const std::string &path) : graphFile(path) {
    if (!(this->graphFile)) {
        ERROR("invalid graph file: ", path);
        throw std::runtime_error("invalid graph file");
    }
}

MTXParser::MTXHeader MTXParser::getHeader() {

    auto tolower = [](const std::string &str) {
        std::string out;
        std::transform(str.begin(), str.end(), std::back_inserter(out), ::tolower);
        return out;
    };

    MTXParser::MTXHeader header;
    std::string identifier, object, format, field, symmetry;
    std::string line;

    Aux::enforceOpened(this->graphFile);
    if (std::getline(this->graphFile, line)) {
        std::istringstream istring(line);
        if (!(istring >> identifier >> object >> format >> field >> symmetry)) {
            throw std::runtime_error("Invalid header format.");
        }
        // the header line must start with %%MatrixMarket or %MatrixMarket
        if (identifier != "%%MatrixMarket" && identifier != "%MatrixMarket") {
            throw std::runtime_error("Invalid identifier.");
        }
        try {
            header.object = objectMap.at(tolower(object));
            header.format = formatMap.at(tolower(format));
            header.field = fieldMap.at(tolower(field));
            header.symmetry = symmetryMap.at(tolower(symmetry));
        } catch (const std::out_of_range &) {
            throw std::runtime_error("Invalid header field.");
        }
        return header;
    } else {
        throw std::runtime_error("Getting MTX header failed.");
    }
}

MTXParser::MatrixSize MTXParser::getMatrixSize() {
    count rows, columns, nonzeros;
    std::string line;

    Aux::enforceOpened(this->graphFile);
    if (std::getline(this->graphFile, line)) {
        while (line[0] == '%') {
            std::getline(this->graphFile, line);
        }
        std::istringstream istring(line);
        if (!(istring >> rows >> columns >> nonzeros)) {
            throw std::runtime_error("Invalid matrix size line format.");
        }
        return MTXParser::MatrixSize(rows, columns, nonzeros);
    } else {
        throw std::runtime_error("Getting MTX matrix size line failed.");
    }
}

bool MTXParser::hasNext() {
    if (!this->hasLine) {
        this->hasLine = static_cast<bool>(std::getline(this->graphFile, this->currentLine));
    }
    return this->hasLine;
}

MTXParser::Edge MTXParser::getNext(bool weighted) {
    if (!this->hasLine)
        throw std::runtime_error("No more lines to be read.");

    do {
        this->hasLine = false;
        // check for comment line starting with '%'
        if (this->currentLine[0] == '%') {
            if (hasNext())
                continue;
            else
                throw std::runtime_error("No more lines to be read.");
        } else {
            return parseLine(this->currentLine, weighted);
        }
    } while (true);
    this->hasLine = false;
    throw std::runtime_error("Invalid MTX file structure.");
}

} /* namespace NetworKit */
