// ast.hpp

#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <variant>

// ── Forward declarations ──────────────────────────────────────────────────────

struct ImportNode;
struct SystemNode;
struct PhysicalNode;
struct MathNode;
struct RodNode;
struct ParticleNode;
struct SpringNode;
struct GravityNode;
struct DeclNode;
struct AssignNode;
struct SimulateNode;
struct InitialNode;
struct RenderNode;

// ── Expression nodes ──────────────────────────────────────────────────────────
// Expressions form their own recursive sub-tree — used inside math blocks

struct ExprNode {
    enum class Kind {
        NUMBER,      // 0.5, 2, 9.8
        IDENTIFIER,  // m1, l1, theta1
        UNARY,       // -x
        BINARY,      // x + y, x * y, x ^ y
        CALL,        // cos(theta1)
        LATEX,       // \partial, \sum etc.
    };

    Kind kind;

    // NUMBER
    double number = 0.0;

    // IDENTIFIER
    std::string name;

    // UNARY — op and single operand
    std::string unaryOp;
    std::unique_ptr<ExprNode> operand;

    // BINARY — op and two operands
    std::string binaryOp;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    // CALL — function name and arguments
    std::string callee;
    std::vector<std::unique_ptr<ExprNode>> args;

    // LATEX — operator name
    std::string latexOp;

    // Every expression node remembers where it came from for error reporting
    int line = 0;
    int col  = 0;
};

// ── Dimension type ────────────────────────────────────────────────────────────
// Represents a composed dimension like [M*L*T^-2]
// Each base dimension carries an exponent — zero means absent

struct DimType {
    int L = 0;  // length
    int M = 0;  // mass
    int T = 0;  // time
    int I = 0;  // current
    int K = 0;  // temperature
    int N = 0;  // amount
    int J = 0;  // luminosity

    bool isDimensionless() const {
        return L == 0 && M == 0 && T == 0 &&
               I == 0 && K == 0 && N == 0 && J == 0;
    }
};

// A dimensioned value — magnitude expression paired with its dimension
struct DimValue {
    std::unique_ptr<ExprNode> magnitude;
    DimType                   dim;
};

// ── Direction ─────────────────────────────────────────────────────────────────

enum class Direction { DOWNWARD, UPWARD, NONE };

// ── Physical layer nodes ──────────────────────────────────────────────────────

struct RodNode {
    std::string name;        // r1, r2
    DimValue    mass;
    DimValue    length;
    std::string pivot;       // "origin" or another rod's name e.g. "r1.end"
    int line = 0;
};

struct ParticleNode {
    std::string name;
    DimValue    mass;
    std::string position;    // "origin" or expression
    int line = 0;
};

struct SpringNode {
    std::string name;
    DimValue    stiffness;   // [M*T^-2]
    DimValue    restLength;  // [L]
    std::string attachA;
    std::string attachB;
    int line = 0;
};

struct GravityNode {
    DimValue  magnitude;
    Direction direction = Direction::DOWNWARD;
    int line = 0;
};

struct PhysicalNode {
    std::vector<RodNode>      rods;
    std::vector<ParticleNode> particles;
    std::vector<SpringNode>   springs;
    std::unique_ptr<GravityNode> gravity;
    int line = 0;
};

// ── Math layer nodes ──────────────────────────────────────────────────────────

// A variable declaration: let theta1: real, theta2: real
struct VarDecl {
    std::string name;
    std::string type;   // "int" or "real"
    int line = 0;
};

struct DeclNode {
    std::vector<VarDecl> vars;
    int line = 0;
};

// An assignment: t = ..., v = ..., lag = ...
struct AssignNode {
    std::string                target;   // t, v, lag, or any identifier
    std::unique_ptr<ExprNode>  expr;
    int line = 0;
};

struct MathNode {
    std::vector<DeclNode>   decls;
    std::vector<AssignNode> assignments;
    int line = 0;
};

// ── System node ───────────────────────────────────────────────────────────────

struct SystemNode {
    std::string                  name;    // DoublePendulum
    std::unique_ptr<PhysicalNode> physical;
    std::unique_ptr<MathNode>     math;
    int line = 0;
};

// ── Simulate node ─────────────────────────────────────────────────────────────

struct InitialCondition {
    std::string                name;
    std::unique_ptr<ExprNode>  value;
};

struct SimulateNode {
    std::string                      systemName;
    DimValue                         duration;
    DimValue                         dt;
    std::vector<InitialCondition>    initial;
    int line = 0;
};

// ── Render node ───────────────────────────────────────────────────────────────

struct RenderNode {
    std::string systemName;
    std::string mode;       // "realtime" or "export"
    int line = 0;
};

// ── Import node ───────────────────────────────────────────────────────────────

struct ImportNode {
    std::string path;       // std.physics.classical
    int line = 0;
};

// ── Program node — root of the entire AST ────────────────────────────────────

struct ProgramNode {
    std::vector<ImportNode>   imports;
    std::vector<SystemNode>   systems;
    std::vector<SimulateNode> simulations;
    std::vector<RenderNode>   renders;
};