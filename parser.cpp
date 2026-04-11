// parser.cpp

#include "parser.hpp"
#include <stdexcept>
#include <string>

// ── Constructor ───────────────────────────────────────────────────────────────

Parser::Parser(const std::vector<Token>& tokens)
    : m_tokens(tokens), m_pos(0) {}

// ── Token navigation ──────────────────────────────────────────────────────────

const Token& Parser::peek() const {
    return m_tokens[m_pos];
}

const Token& Parser::peekNext() const {
    if (m_pos + 1 >= m_tokens.size()) return m_tokens.back();
    return m_tokens[m_pos + 1];
}

const Token& Parser::advance() {
    const Token& t = m_tokens[m_pos];
    if (!isAtEnd()) m_pos++;
    return t;
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& context) {
    if (!check(type))
        throw error("Expected " + context
            + " but got '" + peek().lexeme + "'");
    return advance();
}

// ── Error ─────────────────────────────────────────────────────────────────────

ParseError Parser::error(const std::string& msg) const {
    return ParseError(msg, peek().line, peek().col);
}

// ── Program ───────────────────────────────────────────────────────────────────

ProgramNode Parser::parse() {
    ProgramNode program;

    while (!isAtEnd()) {
        if (check(TokenType::KW_IMPORT))
            program.imports.push_back(parseImport());
        else if (check(TokenType::KW_SYSTEM))
            program.systems.push_back(parseSystem());
        else if (check(TokenType::KW_SIMULATE))
            program.simulations.push_back(parseSimulate());
        else if (check(TokenType::KW_RENDER))
            program.renders.push_back(parseRender());
        else
            throw error("Unexpected token '" + peek().lexeme
                + "' at top level");
    }

    return program;
}

// ── Import ────────────────────────────────────────────────────────────────────

ImportNode Parser::parseImport() {
    ImportNode node;
    node.line = peek().line;
    expect(TokenType::KW_IMPORT, "'import'");
    node.path = expect(TokenType::IMPORT_PATH, "import path").lexeme;
    return node;
}

// ── System ────────────────────────────────────────────────────────────────────

SystemNode Parser::parseSystem() {
    SystemNode node;
    node.line = peek().line;
    expect(TokenType::KW_SYSTEM, "'system'");
    node.name = expect(TokenType::SYSTEM_NAME, "system name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        if (check(TokenType::KW_PHYSICAL))
            node.physical = std::make_unique<PhysicalNode>(parsePhysical());
        else if (check(TokenType::KW_MATH))
            node.math = std::make_unique<MathNode>(parseMath());
        else
            throw error("Expected 'physical' or 'math' block inside system '"
                + node.name + "'");
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

// ── Physical layer ────────────────────────────────────────────────────────────

PhysicalNode Parser::parsePhysical() {
    PhysicalNode node;
    node.line = peek().line;
    expect(TokenType::KW_PHYSICAL, "'physical'");
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        if (check(TokenType::KW_ROD))
            node.rods.push_back(parseRod());
        else if (check(TokenType::KW_PARTICLE))
            node.particles.push_back(parseParticle());
        else if (check(TokenType::KW_SPRING))
            node.springs.push_back(parseSpring());
        else if (check(TokenType::KW_GRAVITY))
            node.gravity = std::make_unique<GravityNode>(parseGravity());
        else
            throw error("Unexpected token '" + peek().lexeme
                + "' in physical block");
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

RodNode Parser::parseRod() {
    RodNode node;
    node.line = peek().line;
    expect(TokenType::KW_ROD, "'rod'");
    node.name = expect(TokenType::IDENTIFIER, "rod name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        std::string field = expect(TokenType::IDENTIFIER,
            "field name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (field == "mass")
            node.mass = parseDimValue();
        else if (field == "length")
            node.length = parseDimValue();
        else if (field == "pivot") {
            // pivot: origin  OR  pivot: r1.end
            if (check(TokenType::KW_ORIGIN)) {
                node.pivot = "origin";
                advance();
            } else {
                // r1.end — consume identifier, dot, identifier
                std::string obj = expect(TokenType::IDENTIFIER,
                    "pivot target").lexeme;
                expect(TokenType::DELIM_DOT, "'.'");
                std::string part = expect(TokenType::IDENTIFIER,
                    "pivot part").lexeme;
                node.pivot = obj + "." + part;
            }
        } else {
            throw error("Unknown rod field '" + field + "'");
        }

        match(TokenType::DELIM_COMMA);
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

ParticleNode Parser::parseParticle() {
    ParticleNode node;
    node.line = peek().line;
    expect(TokenType::KW_PARTICLE, "'particle'");
    node.name = expect(TokenType::IDENTIFIER, "particle name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        std::string field = expect(TokenType::IDENTIFIER,
            "field name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (field == "mass")
            node.mass = parseDimValue();
        else if (field == "position") {
            if (check(TokenType::KW_ORIGIN)) {
                node.position = "origin";
                advance();
            } else {
                node.position = expect(TokenType::IDENTIFIER,
                    "position").lexeme;
            }
        } else {
            throw error("Unknown particle field '" + field + "'");
        }

        match(TokenType::DELIM_COMMA);
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

SpringNode Parser::parseSpring() {
    SpringNode node;
    node.line = peek().line;
    expect(TokenType::KW_SPRING, "'spring'");
    node.name = expect(TokenType::IDENTIFIER, "spring name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        std::string field = expect(TokenType::IDENTIFIER,
            "field name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (field == "stiffness")
            node.stiffness = parseDimValue();
        else if (field == "restlength")
            node.restLength = parseDimValue();
        else if (field == "attacha")
            node.attachA = expect(TokenType::IDENTIFIER,
                "attachment point").lexeme;
        else if (field == "attachb")
            node.attachB = expect(TokenType::IDENTIFIER,
                "attachment point").lexeme;
        else
            throw error("Unknown spring field '" + field + "'");

        match(TokenType::DELIM_COMMA);
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

GravityNode Parser::parseGravity() {
    GravityNode node;
    node.line = peek().line;
    expect(TokenType::KW_GRAVITY, "'gravity'");
    expect(TokenType::DELIM_COLON, "':'");
    node.magnitude = parseDimValue();

    if (check(TokenType::KW_DOWNWARD)) {
        node.direction = Direction::DOWNWARD;
        advance();
    } else if (check(TokenType::KW_UPWARD)) {
        node.direction = Direction::UPWARD;
        advance();
    } else {
        node.direction = Direction::NONE;
    }

    return node;
}

// ── Math layer ────────────────────────────────────────────────────────────────

MathNode Parser::parseMath() {
    MathNode node;
    node.line = peek().line;
    expect(TokenType::KW_MATH, "'math'");
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        if (check(TokenType::KW_LET))
            node.decls.push_back(parseDecl());
        else if (check(TokenType::IDENTIFIER) ||
                 check(TokenType::KW_LET_T)   ||
                 check(TokenType::KW_LET_V)   ||
                 check(TokenType::KW_LAG)) {
            Token target = advance();
            expect(TokenType::OP_EQUALS, "'='");
            node.assignments.push_back(parseAssign(target));
        } else {
            throw error("Unexpected token '" + peek().lexeme
                + "' in math block");
        }
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

DeclNode Parser::parseDecl() {
    DeclNode node;
    node.line = peek().line;
    expect(TokenType::KW_LET, "'let'");

    do {
        VarDecl var;
        var.line = peek().line;
        var.name = expect(TokenType::IDENTIFIER,
            "variable name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (check(TokenType::KW_INT)) {
            var.type = "int";
            advance();
        } else if (check(TokenType::KW_REAL)) {
            var.type = "real";
            advance();
        } else {
            throw error("Expected 'int' or 'real' as variable type");
        }

        node.vars.push_back(std::move(var));
    } while (match(TokenType::DELIM_COMMA));

    return node;
}

AssignNode Parser::parseAssign(const Token& target) {
    AssignNode node;
    node.line   = target.line;
    node.target = target.lexeme;
    node.expr   = parseExpr();
    return node;
}

// ── Simulate ──────────────────────────────────────────────────────────────────

SimulateNode Parser::parseSimulate() {
    SimulateNode node;
    node.line = peek().line;
    expect(TokenType::KW_SIMULATE, "'simulate'");
    node.systemName = expect(TokenType::SYSTEM_NAME, "system name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        std::string field = expect(TokenType::IDENTIFIER,
            "field name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (field == "duration")
            node.duration = parseDimValue();
        else if (field == "dt")
            node.dt = parseDimValue();
        else if (field == "initial") {
            expect(TokenType::DELIM_LBRACE, "'{'");
            while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
                InitialCondition ic;
                ic.name = expect(TokenType::IDENTIFIER,
                    "variable name").lexeme;
                expect(TokenType::DELIM_COLON, "':'");
                ic.value = parseExpr();
                node.initial.push_back(std::move(ic));
                match(TokenType::DELIM_COMMA);
            }
            expect(TokenType::DELIM_RBRACE, "'}'");
        } else {
            throw error("Unknown simulate field '" + field + "'");
        }
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

// ── Render ────────────────────────────────────────────────────────────────────

RenderNode Parser::parseRender() {
    RenderNode node;
    node.line = peek().line;
    expect(TokenType::KW_RENDER, "'render'");
    node.systemName = expect(TokenType::SYSTEM_NAME, "system name").lexeme;
    expect(TokenType::DELIM_LBRACE, "'{'");

    while (!check(TokenType::DELIM_RBRACE) && !isAtEnd()) {
        std::string field = expect(TokenType::IDENTIFIER,
            "field name").lexeme;
        expect(TokenType::DELIM_COLON, "':'");

        if (field == "mode") {
            if (check(TokenType::KW_REALTIME)) {
                node.mode = "realtime";
                advance();
            } else if (check(TokenType::KW_EXPORT)) {
                node.mode = "export";
                advance();
            } else {
                throw error("Expected 'realtime' or 'export' for mode");
            }
        } else {
            throw error("Unknown render field '" + field + "'");
        }
    }

    expect(TokenType::DELIM_RBRACE, "'}'");
    return node;
}

// ── Dimension parsing ─────────────────────────────────────────────────────────

DimValue Parser::parseDimValue() {
    DimValue val;
    val.magnitude = parseExpr();

    // Optional dimension suffix [M*L*T^-2]
    if (check(TokenType::DELIM_LBRACKET)) {
        advance(); // consume '['
        val.dim = parseDimType();
        expect(TokenType::DELIM_RBRACKET, "']'");
    }

    return val;
}

DimType Parser::parseDimType() {
    DimType dim;

    auto applyDim = [&](TokenType t, int exponent) {
        switch (t) {
            case TokenType::DIM_L: dim.L += exponent; break;
            case TokenType::DIM_M: dim.M += exponent; break;
            case TokenType::DIM_T: dim.T += exponent; break;
            case TokenType::DIM_I: dim.I += exponent; break;
            case TokenType::DIM_K: dim.K += exponent; break;
            case TokenType::DIM_N: dim.N += exponent; break;
            case TokenType::DIM_J: dim.J += exponent; break;
            default: break;
        }
    };

    // Parse first dimension symbol — mandatory
    TokenType first = peek().type;
    advance();
    int exp = parseDimExponent();
    applyDim(first, exp);

    // Parse subsequent dimension symbols separated by *
    while (check(TokenType::OP_STAR)) {
        advance(); // consume '*'
        TokenType dim_tok = peek().type;
        advance();
        int e = parseDimExponent();
        applyDim(dim_tok, e);
    }

    return dim;
}

int Parser::parseDimExponent() {
    // Optional ^ followed by optional - and integer
    if (!check(TokenType::OP_CARET)) return 1;
    advance(); // consume '^'

    int sign = 1;
    if (check(TokenType::OP_MINUS)) {
        sign = -1;
        advance();
    }

    const Token& t = expect(TokenType::LIT_INT, "exponent");
    return sign * static_cast<int>(std::get<long long>(t.value));
}

// ── Expression parsing ────────────────────────────────────────────────────────
// Standard recursive descent with correct operator precedence:
// parseExpr → parseAddSub → parseMulDiv → parsePower → parseUnary → parsePrimary

std::unique_ptr<ExprNode> Parser::parseExpr() {
    return parseAddSub();
}

std::unique_ptr<ExprNode> Parser::parseAddSub() {
    auto left = parseMulDiv();

    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
        std::string op = peek().lexeme;
        int line = peek().line;
        int col  = peek().col;
        advance();
        auto right = parseMulDiv();

        auto node       = std::make_unique<ExprNode>();
        node->kind      = ExprNode::Kind::BINARY;
        node->binaryOp  = op;
        node->left      = std::move(left);
        node->right     = std::move(right);
        node->line      = line;
        node->col       = col;
        left            = std::move(node);
    }

    return left;
}

std::unique_ptr<ExprNode> Parser::parseMulDiv() {
    auto left = parsePower();

    while (check(TokenType::OP_STAR) || check(TokenType::OP_SLASH)) {
        std::string op = peek().lexeme;
        int line = peek().line;
        int col  = peek().col;
        advance();
        auto right = parsePower();

        auto node       = std::make_unique<ExprNode>();
        node->kind      = ExprNode::Kind::BINARY;
        node->binaryOp  = op;
        node->left      = std::move(left);
        node->right     = std::move(right);
        node->line      = line;
        node->col       = col;
        left            = std::move(node);
    }

    return left;
}

std::unique_ptr<ExprNode> Parser::parsePower() {
    auto base = parseUnary();

    if (check(TokenType::OP_CARET)) {
        int line = peek().line;
        int col  = peek().col;
        advance();
        // Right-associative — parsePower recurses on right side
        auto exp = parsePower();

        auto node       = std::make_unique<ExprNode>();
        node->kind      = ExprNode::Kind::BINARY;
        node->binaryOp  = "^";
        node->left      = std::move(base);
        node->right     = std::move(exp);
        node->line      = line;
        node->col       = col;
        return node;
    }

    return base;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (check(TokenType::OP_MINUS)) {
        int line = peek().line;
        int col  = peek().col;
        advance();
        auto operand = parseUnary();

        auto node      = std::make_unique<ExprNode>();
        node->kind     = ExprNode::Kind::UNARY;
        node->unaryOp  = "-";
        node->operand  = std::move(operand);
        node->line     = line;
        node->col      = col;
        return node;
    }

    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    // Number literal
    if (check(TokenType::LIT_INT)) {
        auto node    = std::make_unique<ExprNode>();
        node->kind   = ExprNode::Kind::NUMBER;
        node->number = static_cast<double>(
            std::get<long long>(peek().value));
        node->line   = peek().line;
        node->col    = peek().col;
        advance();
        return node;
    }

    if (check(TokenType::LIT_SCI)) {
        auto node    = std::make_unique<ExprNode>();
        node->kind   = ExprNode::Kind::NUMBER;
        node->number = std::get<double>(peek().value);
        node->line   = peek().line;
        node->col    = peek().col;
        advance();
        return node;
    }

    // Parenthesised expression
    if (check(TokenType::DELIM_LPAREN)) {
        advance();
        auto expr = parseExpr();
        expect(TokenType::DELIM_RPAREN, "')'");
        return expr;
    }

    // Identifier or function call
    if (check(TokenType::IDENTIFIER) ||
        check(TokenType::KW_LET_T)   ||
        check(TokenType::KW_LET_V)   ||
        check(TokenType::KW_LAG)) {

        std::string name = peek().lexeme;
        int line = peek().line;
        int col  = peek().col;
        advance();

        // Function call — cos(theta1)
        if (check(TokenType::DELIM_LPAREN)) {
            advance();
            auto node    = std::make_unique<ExprNode>();
            node->kind   = ExprNode::Kind::CALL;
            node->callee = name;
            node->line   = line;
            node->col    = col;

            while (!check(TokenType::DELIM_RPAREN) && !isAtEnd()) {
                node->args.push_back(parseExpr());
                match(TokenType::DELIM_COMMA);
            }

            expect(TokenType::DELIM_RPAREN, "')'");
            return node;
        }

        // Plain identifier
        auto node  = std::make_unique<ExprNode>();
        node->kind = ExprNode::Kind::IDENTIFIER;
        node->name = name;
        node->line = line;
        node->col  = col;
        return node;
    }

    // LaTeX operator
    if (check(TokenType::LATEX_PARTIAL) ||
        check(TokenType::LATEX_INTEGRAL)||
        check(TokenType::LATEX_SUM)     ||
        check(TokenType::LATEX_SQRT)    ||
        check(TokenType::LATEX_INF)) {

        auto node      = std::make_unique<ExprNode>();
        node->kind     = ExprNode::Kind::LATEX;
        node->latexOp  = peek().lexeme;
        node->line     = peek().line;
        node->col      = peek().col;
        advance();
        return node;
    }

    throw error("Expected expression but got '" + peek().lexeme + "'");
}