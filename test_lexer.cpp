// test_lexer.cpp

#include "lexer.hpp"
#include "token.hpp"
#include <iostream>
#include <iomanip>
#include <string>

// ── Token type to string ──────────────────────────────────────────────────────

static std::string tokenTypeName(TokenType type) {
    switch (type) {
        // Literals
        case TokenType::LIT_INT:         return "LIT_INT";
        case TokenType::LIT_REAL:        return "LIT_REAL";
        case TokenType::LIT_SCI:         return "LIT_SCI";

        // Dimensions
        case TokenType::DIM_L:           return "DIM_L";
        case TokenType::DIM_M:           return "DIM_M";
        case TokenType::DIM_T:           return "DIM_T";
        case TokenType::DIM_I:           return "DIM_I";
        case TokenType::DIM_K:           return "DIM_K";
        case TokenType::DIM_N:           return "DIM_N";
        case TokenType::DIM_J:           return "DIM_J";

        // Structure keywords
        case TokenType::KW_SYSTEM:       return "KW_SYSTEM";
        case TokenType::KW_PHYSICAL:     return "KW_PHYSICAL";
        case TokenType::KW_MATH:         return "KW_MATH";
        case TokenType::KW_SIMULATE:     return "KW_SIMULATE";
        case TokenType::KW_RENDER:       return "KW_RENDER";
        case TokenType::KW_IMPORT:       return "KW_IMPORT";
        case TokenType::KW_LET:          return "KW_LET";

        // Physical keywords
        case TokenType::KW_ROD:          return "KW_ROD";
        case TokenType::KW_PARTICLE:     return "KW_PARTICLE";
        case TokenType::KW_SPRING:       return "KW_SPRING";
        case TokenType::KW_GRAVITY:      return "KW_GRAVITY";
        case TokenType::KW_ORIGIN:       return "KW_ORIGIN";
        case TokenType::KW_DOWNWARD:     return "KW_DOWNWARD";
        case TokenType::KW_UPWARD:       return "KW_UPWARD";

        // Math keywords
        case TokenType::KW_LAG:          return "KW_LAG";
        case TokenType::KW_LET_T:        return "KW_LET_T";
        case TokenType::KW_LET_V:        return "KW_LET_V";

        // Simulation keywords
        case TokenType::KW_DURATION:     return "KW_DURATION";
        case TokenType::KW_DT:           return "KW_DT";
        case TokenType::KW_INITIAL:      return "KW_INITIAL";
        case TokenType::KW_MODE:         return "KW_MODE";
        case TokenType::KW_REALTIME:     return "KW_REALTIME";
        case TokenType::KW_EXPORT:       return "KW_EXPORT";

        // Type keywords
        case TokenType::KW_INT:          return "KW_INT";
        case TokenType::KW_REAL:         return "KW_REAL";

        // LaTeX
        case TokenType::LATEX_PARTIAL:   return "LATEX_PARTIAL";
        case TokenType::LATEX_INTEGRAL:  return "LATEX_INTEGRAL";
        case TokenType::LATEX_SUM:       return "LATEX_SUM";
        case TokenType::LATEX_SQRT:      return "LATEX_SQRT";
        case TokenType::LATEX_INF:       return "LATEX_INF";

        // Operators
        case TokenType::OP_PLUS:         return "OP_PLUS";
        case TokenType::OP_MINUS:        return "OP_MINUS";
        case TokenType::OP_STAR:         return "OP_STAR";
        case TokenType::OP_CARET:        return "OP_CARET";
        case TokenType::OP_SLASH:        return "OP_SLASH";
        case TokenType::OP_EQUALS:       return "OP_EQUALS";

        // Delimiters
        case TokenType::DELIM_LBRACE:    return "DELIM_LBRACE";
        case TokenType::DELIM_RBRACE:    return "DELIM_RBRACE";
        case TokenType::DELIM_LBRACKET:  return "DELIM_LBRACKET";
        case TokenType::DELIM_RBRACKET:  return "DELIM_RBRACKET";
        case TokenType::DELIM_LPAREN:    return "DELIM_LPAREN";
        case TokenType::DELIM_RPAREN:    return "DELIM_RPAREN";
        case TokenType::DELIM_COMMA:     return "DELIM_COMMA";
        case TokenType::DELIM_COLON:     return "DELIM_COLON";
        case TokenType::DELIM_DOT:       return "DELIM_DOT";

        // Misc
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::COMMENT:         return "COMMENT";
        case TokenType::SYSTEM_NAME:     return "SYSTEM_NAME";
        case TokenType::NEWLINE:         return "NEWLINE";
        case TokenType::END_OF_FILE:     return "END_OF_FILE";
        case TokenType::UNKNOWN:         return "UNKNOWN";

        default:                         return "???";
    }
}

// ── Value printer ─────────────────────────────────────────────────────────────

static std::string tokenValue(const Token& tok) {
    if (std::holds_alternative<long long>(tok.value))
        return "= " + std::to_string(std::get<long long>(tok.value));
    if (std::holds_alternative<double>(tok.value))
        return "= " + std::to_string(std::get<double>(tok.value));
    return "";
}

// ── Pretty printer ────────────────────────────────────────────────────────────

static void printTokens(const std::vector<Token>& tokens) {
    // Column headers
    std::cout
        << std::left
        << std::setw(6)  << "LINE"
        << std::setw(6)  << "COL"
        << std::setw(22) << "TYPE"
        << std::setw(24) << "LEXEME"
        << "VALUE"
        << "\n"
        << std::string(70, '-') << "\n";

    for (const auto& tok : tokens) {
        std::cout
            << std::left
            << std::setw(6)  << tok.line
            << std::setw(6)  << tok.col
            << std::setw(22) << tokenTypeName(tok.type)
            << std::setw(24) << ("'" + tok.lexeme + "'")
            << tokenValue(tok)
            << "\n";

        // Stop printing after EOF
        if (tok.type == TokenType::END_OF_FILE) break;
    }
}

// ── Error checker ─────────────────────────────────────────────────────────────

static bool hasErrors(const std::vector<Token>& tokens) {
    bool found = false;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::UNKNOWN) {
            std::cerr
                << "[LEXER ERROR] line " << tok.line
                << ", col " << tok.col
                << ": " << tok.lexeme << "\n";
            found = true;
        }
    }
    return found;
}

// ── Test source ───────────────────────────────────────────────────────────────

static const std::string TEST_SOURCE = R"(
import std.physics.classical
import std.render.2d

system DoublePendulum {

    # Physical layer
    physical {
        rod r1 { mass: 1.0 [M], length: 1.2 [L], pivot: origin }
        rod r2 { mass: 0.8 [M], length: 0.9 [L], pivot: r1.end }
        gravity: 9.8 [L*T^-2] downward
    }

    # Mathematical layer
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
    std::cout << "=== Noether Lexer Test ===\n\n";
    std::cout << "── Source ───────────────────────────────────────────────\n";
    std::cout << TEST_SOURCE << "\n";
    std::cout << "── Token Stream ─────────────────────────────────────────\n";

    Lexer lexer(TEST_SOURCE);
    std::vector<Token> tokens = lexer.tokenise();

    if (hasErrors(tokens)) {
        std::cerr << "\nLexer produced errors. See above.\n";
        return 1;
    }

    printTokens(tokens);

    std::cout << "\nTotal tokens: " << tokens.size() << "\n";
    std::cout << "\n=== All OK ===\n";
    return 0;
}