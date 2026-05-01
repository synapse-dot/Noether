// dash.cpp — The Noether CLI

#include "lexer.hpp"
#include "parser.hpp"
#include "analyser.hpp"
#include "reconciler.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

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

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: dash <file.noe>\n";
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        // 1. Lex
        Lexer lexer(source);
        auto tokens = lexer.tokenise();

        // 2. Parse
        Parser parser(tokens);
        auto program = parser.parse();

        // 3. Analyse
        Analyser analyser(program);
        analyser.analyse();

        // 4. Run Interpreter (includes Reconciler)
        Interpreter interpreter(program);
        interpreter.run();

    } catch (const ParseError& e) {
        std::cerr << "[PARSE ERROR] line " << e.line << ": " << e.what() << "\n";
        return 1;
    } catch (const AnalysisError& e) {
        std::cerr << "[ANALYSIS ERROR] line " << e.line << ": " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
