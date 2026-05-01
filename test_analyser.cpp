// test_analyser.cpp

#include "lexer.hpp"
#include "parser.hpp"
#include "analyser.hpp"
#include <iostream>
#include <memory>
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

static bool expectAnalysisFailure(const ProgramNode& program,
                                  const std::string& expectedSubstr) {
    Analyser analyser(program);
    try {
        analyser.analyse();
        std::cerr << "Expected analysis failure containing: " << expectedSubstr << "\n";
        return false;
    } catch (const AnalysisError& e) {
        return std::string(e.what()).find(expectedSubstr) != std::string::npos;
    }
}

static std::unique_ptr<ExprNode> number(double value) {
    auto node = std::make_unique<ExprNode>();
    node->kind = ExprNode::Kind::NUMBER;
    node->number = value;
    return node;
}

static ProgramNode parseProgram() {
    Lexer lexer(TEST_SOURCE);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    return parser.parse();
}

int main() {
    std::cout << "=== Noether Analyser Test ===\n\n";

    Lexer lexer(TEST_SOURCE);
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenise();
    } catch (const std::exception& e) {
        std::cerr << "[LEXER ERROR] " << e.what() << "\n";
        return 1;
    }

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

    // Base source should analyse successfully.
    try {
        Analyser analyser(program);
        analyser.analyse();
    } catch (const AnalysisError& e) {
        std::cerr << "[ANALYSIS ERROR] line " << e.line
                  << ": " << e.what() << "\n";
        return 1;
    }

    // Invalid integrator inside a system simulate block.
    {
        ProgramNode caseProgram = parseProgram();
        auto simulate = std::make_unique<SimulateBlockNode>();
        simulate->integrator = "verlet";
        simulate->timestep.magnitude = number(0.01);
        simulate->timestep.dim.T = 1;
        simulate->duration.magnitude = number(10.0);
        simulate->duration.dim.T = 1;
        simulate->line = 1;
        caseProgram.systems[0].simulate = std::move(simulate);

        if (!expectAnalysisFailure(caseProgram, "unsupported integrator")) {
            std::cerr << "[FAIL] invalid integrator test\n";
            return 1;
        }
    }

    // Missing symbol in visualize plot directive.
    {
        ProgramNode caseProgram = parseProgram();
        auto visualize = std::make_unique<VisualizeBlockNode>();
        visualize->scene_name = "s";
        visualize->plots.push_back(PlotDirective{"theta1", "missing_symbol", 1});
        caseProgram.systems[0].visualize = std::move(visualize);

        if (!expectAnalysisFailure(caseProgram, "undefined symbol")) {
            std::cerr << "[FAIL] missing symbol plot test\n";
            return 1;
        }
    }

    // Negative timestep in system simulate block.
    {
        ProgramNode caseProgram = parseProgram();
        auto simulate = std::make_unique<SimulateBlockNode>();
        simulate->integrator = "rk4";
        simulate->timestep.magnitude = number(-0.01);
        simulate->timestep.dim.T = 1;
        simulate->duration.magnitude = number(10.0);
        simulate->duration.dim.T = 1;
        simulate->line = 1;
        caseProgram.systems[0].simulate = std::move(simulate);

        if (!expectAnalysisFailure(caseProgram, "timestep must be > 0")) {
            std::cerr << "[FAIL] negative timestep test\n";
            return 1;
        }
    }

    std::cout << "=== All OK ===\n";
    return 0;
}
