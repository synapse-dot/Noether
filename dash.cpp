#include "lexer.hpp"
#include "parser.hpp"
#include "analyser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Unable to open file: " + path);
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

int runPipeline(const std::string& source) {
    Lexer lexer(source);
    const auto tokens = lexer.tokenise();

    Parser parser(tokens);
    const ProgramNode program = parser.parse();

    Analyser analyser(program);
    analyser.analyse();

    std::cout << "Noether v1 pipeline success:\n";
    std::cout << "  imports: " << program.imports.size() << "\n";
    std::cout << "  systems: " << program.systems.size() << "\n";
    std::cout << "  simulate blocks: " << program.simulations.size() << "\n";
    std::cout << "  render blocks: " << program.renders.size() << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: dash <file.noe>\n";
        return 64;
    }

    try {
        const std::string source = readFile(argv[1]);
        return runPipeline(source);
    } catch (const ParseError& e) {
        std::cerr << "[PARSE ERROR] line " << e.line << ", col " << e.col
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const AnalysisError& e) {
        std::cerr << "[ANALYSIS ERROR] line " << e.line << ", col " << e.col
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
