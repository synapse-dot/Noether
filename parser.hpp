// parser.hpp

#pragma once
#include "token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

// ── Parse error ───────────────────────────────────────────────────────────────

struct ParseError : public std::runtime_error {
    int line;
    int col;
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

// ── Parser ────────────────────────────────────────────────────────────────────

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    ProgramNode parse();

private:
    const std::vector<Token>& m_tokens;
    std::size_t                m_pos;

    // ── Token navigation ──────────────────────────────────────────
    const Token& peek() const;
    const Token& peekNext() const;
    const Token& advance();
    bool         check(TokenType type) const;
    bool         isAtEnd() const;
    const Token& expect(TokenType type, const std::string& context);
    bool         match(TokenType type);

    // ── Error ─────────────────────────────────────────────────────
    ParseError error(const std::string& msg) const;

    // ── Top-level parsers ─────────────────────────────────────────
    ImportNode   parseImport();
    SystemNode   parseSystem();
    SimulateNode parseSimulate();
    SimulateBlockNode parseSimulateBlock();
    VisualizeBlockNode parseVisualizeBlock();
    RenderNode   parseRender();

    // ── Physical layer ────────────────────────────────────────────
    PhysicalNode  parsePhysical();
    RodNode       parseRod();
    ParticleNode  parseParticle();
    SpringNode    parseSpring();
    GravityNode   parseGravity();

    // ── Math layer ────────────────────────────────────────────────
    MathNode   parseMath();
    DeclNode   parseDecl();
    AssignNode parseAssign(const Token& target);

    // ── Expressions ───────────────────────────────────────────────
    std::unique_ptr<ExprNode> parseExpr();
    std::unique_ptr<ExprNode> parseAddSub();
    std::unique_ptr<ExprNode> parseMulDiv();
    std::unique_ptr<ExprNode> parsePower();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parsePrimary();

    // ── Dimensions ────────────────────────────────────────────────
    DimValue parseDimValue();
    DimType  parseDimType();
    int      parseDimExponent();
};
