// test_parser.cpp

#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include <iostream>
#include <string>

// ── Indentation helper ────────────────────────────────────────────────────────

static std::string indent(int depth) {
    return std::string(depth * 2, ' ');
}

// ── Forward declarations ──────────────────────────────────────────────────────

static void printExpr(const ExprNode& expr, int depth);
static void printDimValue(const DimValue& val, int depth);
static void printDimType(const DimType& dim, int depth);
static void printPhysical(const PhysicalNode& node, int depth);
static void printMath(const MathNode& node, int depth);
static void printSystem(const SystemNode& node, int depth);
static void printSimulate(const SimulateNode& node, int depth);
static void printRender(const RenderNode& node, int depth);

// ── Expression printer ────────────────────────────────────────────────────────

static void printExpr(const ExprNode& expr, int depth) {
    switch (expr.kind) {
        case ExprNode::Kind::NUMBER:
            std::cout << indent(depth)
                      << "NUMBER(" << expr.number << ")\n";
            break;

        case ExprNode::Kind::IDENTIFIER:
            std::cout << indent(depth)
                      << "IDENTIFIER(" << expr.name << ")\n";
            break;

        case ExprNode::Kind::UNARY:
            std::cout << indent(depth)
                      << "UNARY(" << expr.unaryOp << ")\n";
            printExpr(*expr.operand, depth + 1);
            break;

        case ExprNode::Kind::BINARY:
            std::cout << indent(depth)
                      << "BINARY(" << expr.binaryOp << ")\n";
            printExpr(*expr.left,  depth + 1);
            printExpr(*expr.right, depth + 1);
            break;

        case ExprNode::Kind::CALL:
            std::cout << indent(depth)
                      << "CALL(" << expr.callee << ")\n";
            for (const auto& arg : expr.args)
                printExpr(*arg, depth + 1);
            break;

        case ExprNode::Kind::LATEX:
            std::cout << indent(depth)
                      << "LATEX(" << expr.latexOp << ")\n";
            break;
    }
}

// ── Dimension printers ────────────────────────────────────────────────────────

static void printDimType(const DimType& dim, int depth) {
    std::string s;
    auto append = [&](const char* sym, int exp) {
        if (exp == 0) return;
        if (!s.empty()) s += "*";
        s += sym;
        if (exp != 1) s += "^" + std::to_string(exp);
    };
    append("L", dim.L);
    append("M", dim.M);
    append("T", dim.T);
    append("I", dim.I);
    append("K", dim.K);
    append("N", dim.N);
    append("J", dim.J);
    if (s.empty()) s = "dimensionless";
    std::cout << indent(depth) << "DIM[" << s << "]\n";
}

static void printDimValue(const DimValue& val, int depth) {
    std::cout << indent(depth) << "DimValue:\n";
    if (val.magnitude)
        printExpr(*val.magnitude, depth + 1);
    else
        std::cout << indent(depth + 1) << "(no magnitude)\n";
    printDimType(val.dim, depth + 1);
}

// ── Physical printer ──────────────────────────────────────────────────────────

static void printPhysical(const PhysicalNode& node, int depth) {
    std::cout << indent(depth) << "PhysicalNode:\n";

    for (const auto& rod : node.rods) {
        std::cout << indent(depth + 1) << "Rod: " << rod.name << "\n";
        std::cout << indent(depth + 2) << "mass:\n";
        printDimValue(rod.mass, depth + 3);
        std::cout << indent(depth + 2) << "length:\n";
        printDimValue(rod.length, depth + 3);
        std::cout << indent(depth + 2) << "pivot: "
                  << rod.pivot << "\n";
    }

    for (const auto& p : node.particles) {
        std::cout << indent(depth + 1) << "Particle: " << p.name << "\n";
        std::cout << indent(depth + 2) << "mass:\n";
        printDimValue(p.mass, depth + 3);
        std::cout << indent(depth + 2) << "position: "
                  << p.position << "\n";
    }

    for (const auto& s : node.springs) {
        std::cout << indent(depth + 1) << "Spring: " << s.name << "\n";
        std::cout << indent(depth + 2) << "stiffness:\n";
        printDimValue(s.stiffness, depth + 3);
        std::cout << indent(depth + 2) << "restLength:\n";
        printDimValue(s.restLength, depth + 3);
        std::cout << indent(depth + 2) << "attachA: " << s.attachA << "\n";
        std::cout << indent(depth + 2) << "attachB: " << s.attachB << "\n";
    }

    if (node.gravity) {
        std::cout << indent(depth + 1) << "Gravity:\n";
        printDimValue(node.gravity->magnitude, depth + 2);
        std::string dir =
            node.gravity->direction == Direction::DOWNWARD ? "downward" :
            node.gravity->direction == Direction::UPWARD   ? "upward"   :
                                                             "none";
        std::cout << indent(depth + 2) << "direction: " << dir << "\n";
    }
}

// ── Math printer ──────────────────────────────────────────────────────────────

static void printMath(const MathNode& node, int depth) {
    std::cout << indent(depth) << "MathNode:\n";

    for (const auto& decl : node.decls) {
        std::cout << indent(depth + 1) << "Decl:\n";
        for (const auto& var : decl.vars)
            std::cout << indent(depth + 2)
                      << var.name << " : " << var.type << "\n";
    }

    for (const auto& assign : node.assignments) {
        std::cout << indent(depth + 1)
                  << "Assign: " << assign.target << " =\n";
        if (assign.expr)
            printExpr(*assign.expr, depth + 2);
    }
}

// ── System printer ────────────────────────────────────────────────────────────

static void printSystem(const SystemNode& node, int depth) {
    std::cout << indent(depth) << "SystemNode: " << node.name << "\n";
    if (node.physical)
        printPhysical(*node.physical, depth + 1);
    if (node.math)
        printMath(*node.math, depth + 1);
}

// ── Simulate printer ──────────────────────────────────────────────────────────

static void printSimulate(const SimulateNode& node, int depth) {
    std::cout << indent(depth)
              << "SimulateNode: " << node.systemName << "\n";
    std::cout << indent(depth + 1) << "duration:\n";
    printDimValue(node.duration, depth + 2);
    std::cout << indent(depth + 1) << "dt:\n";
    printDimValue(node.dt, depth + 2);
    std::cout << indent(depth + 1) << "initial:\n";
    for (const auto& ic : node.initial) {
        std::cout << indent(depth + 2) << ic.name << " =\n";
        if (ic.value)
            printExpr(*ic.value, depth + 3);
    }
}

// ── Render printer ────────────────────────────────────────────────────────────

static void printRender(const RenderNode& node, int depth) {
    std::cout << indent(depth)
              << "RenderNode: " << node.systemName << "\n";
    std::cout << indent(depth + 1) << "mode: " << node.mode << "\n";
}

// ── Test source ───────────────────────────────────────────────────────────────

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

// ── Entry point ───────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== Noether Parser Test ===\n\n";

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

    // Print AST
    std::cout << "ProgramNode:\n";

    std::cout << indent(1) << "Imports:\n";
    for (const auto& imp : program.imports)
        std::cout << indent(2) << imp.path << "\n";

    for (const auto& sys : program.systems)
        printSystem(sys, 1);

    for (const auto& sim : program.simulations)
        printSimulate(sim, 1);

    for (const auto& ren : program.renders)
        printRender(ren, 1);

    std::cout << "\n=== All OK ===\n";
    return 0;
}