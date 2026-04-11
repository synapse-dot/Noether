// test_analyser.cpp

#include "lexer.hpp"
#include "parser.hpp"
#include "analyser.hpp"
#include <iostream>
#include <string>

static const std::string TEST_SOURCE = R"(
import std.physics.classical
import std.render.2d

system DoublePendulum {

    physical {
        rod r1 { mass: 1.0 [M], length: 1.2 [L], pivot: origin }
        rod r2 { mass: 0.8 [M], length: 0.9 [L], pivot: r1.end }
        gravity: 9.8 [L*T^-2] downward
    }

    math {
        let theta1: real, theta2: real

        t = 0.5 * m1 * l1^2 * theta1_dot^2
          + 0.5 * m2 * (l1^2 * theta1_dot^2
          + l2^2 * theta2_dot^2
          + 2 * l1 * l2 * theta1_dot * theta2_dot * cos(theta1 - theta2))

        v = -(m1 + m2) * g * l1 * cos(theta1)
          - m2 * g * l2 * cos(theta2)

        lag = t - v
    }
}

simulate DoublePendulum {
    duration: 30 [T]
    dt: 0.01 [T]
    initial { theta1: 0.5, theta2: 1.2 }
}

render DoublePendulum { mode: realtime }
)";

int main() {
    std::cout << "=== Noether Analyser Test ===\n\n";

    // Lex
    Lexer lexer(TEST_SOURCE);
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenise();
    } catch (const std::exception& e) {
        std::cerr << "[LEXER ERROR] " << e.what() << "\n";
        return 1;
    }

    // Parse
    Parser parser(tokens);
    ProgramNode program;
    try {
        program = parser.parse();
    } catch (const ParseError& e) {
        std::cerr << "[PARSE ERROR] line " << e.line
                  << ", col " << e.col
                  << ": " << e.what() << "\n";
        return 1;
    }

    // Analyse
    Analyser analyser(program);
    try {
        analyser.analyse();
    } catch (const AnalysisError& e) {
        std::cerr << "[ANALYSIS ERROR] line " << e.line
                  << ": " << e.what() << "\n";
        return 1;
    }

    std::cout << "=== All OK ===\n";
    return 0;
}