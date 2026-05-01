// lexer.cpp

#include "lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <cmath>

// ── Keyword table ─────────────────────────────────────────────────────────────

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    { "system",     TokenType::KW_SYSTEM   },
    { "physical",   TokenType::KW_PHYSICAL },
    { "math",       TokenType::KW_MATH     },
    { "simulate",   TokenType::KW_SIMULATE },
    { "render",     TokenType::KW_RENDER   },
    { "import",     TokenType::KW_IMPORT   },
    { "let",        TokenType::KW_LET      },
    { "rod",        TokenType::KW_ROD      },
    { "particle",   TokenType::KW_PARTICLE },
    { "spring",     TokenType::KW_SPRING   },
    { "gravity",    TokenType::KW_GRAVITY  },
    { "origin",     TokenType::KW_ORIGIN   },
    { "downward",   TokenType::KW_DOWNWARD },
    { "upward",     TokenType::KW_UPWARD   },
    { "lag",        TokenType::KW_LAG      },
    { "duration",   TokenType::KW_DURATION },
    { "dt",         TokenType::KW_DT       },
    { "initial",    TokenType::KW_INITIAL  },
    { "mode",       TokenType::KW_MODE     },
    { "realtime",   TokenType::KW_REALTIME },
    { "export",     TokenType::KW_EXPORT   },
    { "integrator", TokenType::KW_INTEGRATOR },
    { "timestep",   TokenType::KW_TIMESTEP },
    { "scene",      TokenType::KW_SCENE },
    { "plot",       TokenType::KW_PLOT },
    { "vs",         TokenType::KW_VS },
    { "int",        TokenType::KW_INT      },
    { "real",       TokenType::KW_REAL     },
};

// Reserved single-letter math symbols — only meaningful inside math blocks.
// The parser will reinterpret these contextually; the lexer just flags them.
static const std::unordered_map<std::string, TokenType> MATH_RESERVED = {
    { "t",  TokenType::KW_LET_T },
    { "v",  TokenType::KW_LET_V },
};

// LaTeX-inspired operator table
static const std::unordered_map<std::string, TokenType> LATEX_KEYWORDS = {
    { "partial",  TokenType::LATEX_PARTIAL  },
    { "integral", TokenType::LATEX_INTEGRAL },
    { "sum",      TokenType::LATEX_SUM      },
    { "sqrt",     TokenType::LATEX_SQRT     },
    { "inf",      TokenType::LATEX_INF      },
};

// ── Constructor ───────────────────────────────────────────────────────────────

Lexer::Lexer(const std::string& source)
    : m_source(source), m_pos(0), m_line(1), m_col(1) {}

// ── Character navigation ──────────────────────────────────────────────────────

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_pos];
}

char Lexer::peekNext() const {
    if (m_pos + 1 >= m_source.size()) return '\0';
    return m_source[m_pos + 1];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') { m_line++; m_col = 1; }
    else           { m_col++;             }
    return c;
}

bool Lexer::isAtEnd() const {
    return m_pos >= m_source.size();
}

// ── Token construction ────────────────────────────────────────────────────────

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return Token{ type, lexeme, m_line, m_col, std::monostate{} };
}

Token Lexer::errorToken(const std::string& msg) const {
    return Token{ TokenType::UNKNOWN, msg, m_line, m_col, std::monostate{} };
}

// ── Whitespace ────────────────────────────────────────────────────────────────

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else
            break;
    }
}

// ── Comment scanning ──────────────────────────────────────────────────────────

Token Lexer::scanComment() {
    // Consume everything from # to end of line
    std::string lexeme;
    while (!isAtEnd() && peek() != '\n')
        lexeme += advance();
    return makeToken(TokenType::COMMENT, lexeme);
}

Token Lexer::scanImportPath() {
    std::string lexeme;

    // Consume the full path — letters, digits, dots, underscores
    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(c) || c == '.' || c == '_')
            lexeme += advance();
        else
            break;
    }

    if (lexeme.empty())
        return errorToken("Expected import path after 'import' at line "
            + std::to_string(m_line));

    return makeToken(TokenType::IMPORT_PATH, lexeme);
}

Token Lexer::scanString() {
    std::string lexeme;
    while (!isAtEnd() && peek() != '"')
        lexeme += advance();
    if (isAtEnd())
        return errorToken("Unterminated string literal");
    advance(); // closing quote
    return makeToken(TokenType::LIT_STRING, lexeme);
}

// ── Number scanning ───────────────────────────────────────────────────────────

Token Lexer::scanNumber() {
    m_expectSystemName = false;
    m_expectImportPath = false;
    std::string lexeme;
    bool isReal = false;
    // bool hasSciNotation = false;

    // Integer or decimal part
    while (!isAtEnd() && std::isdigit(peek()))
        lexeme += advance();

    // Decimal point
    if (!isAtEnd() && peek() == '.' && std::isdigit(peekNext())) {
        isReal = true;
        lexeme += advance(); // consume '.'
        while (!isAtEnd() && std::isdigit(peek()))
            lexeme += advance();
    }

    // Scientific notation — e or E
    if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        // hasSciNotation = true;
        isReal = true;
        lexeme += advance(); // consume 'e'
        if (!isAtEnd() && (peek() == '+' || peek() == '-'))
            lexeme += advance(); // consume sign
        while (!isAtEnd() && std::isdigit(peek()))
            lexeme += advance();
    }

    // Normalise to scientific notation internally
    double numericValue = std::stod(lexeme);
    Token tok;

    if (!isReal) {
        // Pure integer — store as long long
        tok = makeToken(TokenType::LIT_INT, lexeme);
        tok.value = static_cast<long long>(numericValue);
    } else {
        // Real — store as double, mark as LIT_SCI after normalisation
        tok = makeToken(TokenType::LIT_SCI, lexeme);
        tok.value = numericValue;
    }

    return tok;
}

// ── Identifier / keyword scanning ────────────────────────────────────────────

Token Lexer::scanIdentifierOrKeyword() {
    std::string lexeme;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_'))
        lexeme += advance();

    // Check dimension symbols first — single uppercase letters
    if (lexeme.size() == 1 && std::isupper(lexeme[0]))
        return scanDimension(lexeme[0]);

    // Check keyword table
    auto kwIt = KEYWORDS.find(lexeme);
    if (kwIt != KEYWORDS.end()) {
        TokenType type = kwIt->second;

        // Flag that the next identifier should be a system name
        if (type == TokenType::KW_SYSTEM   ||
            type == TokenType::KW_SIMULATE ||
            type == TokenType::KW_RENDER)
            m_expectSystemName = true;

        if (type == TokenType::KW_IMPORT)   // ← add this block
            m_expectImportPath = true;
                return makeToken(type, lexeme);
            }

    // Check math-reserved single letters
    auto mathIt = MATH_RESERVED.find(lexeme);
    if (mathIt != MATH_RESERVED.end())
        return makeToken(mathIt->second, lexeme);

    // If we're expecting a system name, allow PascalCase
    if (m_expectSystemName) {
        m_expectSystemName = false;

        bool pascal = std::isupper(lexeme[0]);
        for (char c : lexeme)
            if (!std::isalnum(c) && c != '_')
                return errorToken(
                    "System name '" + lexeme
                    + "' may only contain letters, digits, and underscores at line "
                    + std::to_string(m_line)
                );
        if (pascal) return makeToken(TokenType::SYSTEM_NAME, lexeme);
        return makeToken(TokenType::IDENTIFIER, lexeme);
    }

    // Otherwise enforce lowercase for all user identifiers
    for (char c : lexeme)
        if (std::isupper(c))
            return errorToken(
                "Identifier '" + lexeme + "' must be lowercase at line "
                + std::to_string(m_line)
            );

    return makeToken(TokenType::IDENTIFIER, lexeme);
}
// ── Dimension symbol scanning ─────────────────────────────────────────────────

Token Lexer::scanDimension(char c) {
    switch (c) {
        case 'L': return makeToken(TokenType::DIM_L, "L");
        case 'M': return makeToken(TokenType::DIM_M, "M");
        case 'T': return makeToken(TokenType::DIM_T, "T");
        case 'I': return makeToken(TokenType::DIM_I, "I");
        case 'K': return makeToken(TokenType::DIM_K, "K");
        case 'N': return makeToken(TokenType::DIM_N, "N");
        case 'J': return makeToken(TokenType::DIM_J, "J");
        default:
            return errorToken(
                std::string("Unknown dimension symbol '") + c + "'"
            );
    }
}

// ── LaTeX operator scanning ───────────────────────────────────────────────────

Token Lexer::scanLatex() {
    // Consume the backslash, then read the keyword
    std::string lexeme;
    while (!isAtEnd() && std::isalpha(peek()))
        lexeme += advance();

    auto it = LATEX_KEYWORDS.find(lexeme);
    if (it != LATEX_KEYWORDS.end())
        return makeToken(it->second, "\\" + lexeme);

    return errorToken("Unknown LaTeX operator '\\" + lexeme + "'");
}

// ── Symbol scanning ───────────────────────────────────────────────────────────

Token Lexer::scanSymbol() {
    m_expectSystemName = false;
    m_expectImportPath = false;
    char c = advance();
    switch (c) {
        case '+': return makeToken(TokenType::OP_PLUS,        "+");
        case '-': return makeToken(TokenType::OP_MINUS,       "-");
        case '*': return makeToken(TokenType::OP_STAR,        "*");
        case '^': return makeToken(TokenType::OP_CARET,       "^");
        case '/': return makeToken(TokenType::OP_SLASH,       "/");
        case '=': return makeToken(TokenType::OP_EQUALS,      "=");
        case '{': return makeToken(TokenType::DELIM_LBRACE,   "{");
        case '}': return makeToken(TokenType::DELIM_RBRACE,   "}");
        case '[': return makeToken(TokenType::DELIM_LBRACKET, "[");
        case ']': return makeToken(TokenType::DELIM_RBRACKET, "]");
        case '(': return makeToken(TokenType::DELIM_LPAREN,   "(");
        case ')': return makeToken(TokenType::DELIM_RPAREN,   ")");
        case ',': return makeToken(TokenType::DELIM_COMMA,    ",");
        case ':': return makeToken(TokenType::DELIM_COLON,    ":");
        case '.': return makeToken(TokenType::DELIM_DOT,      ".");
        default:
            return errorToken(
                std::string("Unexpected character '") + c + "'"
            );
    }
}

// ── Main tokenise loop ────────────────────────────────────────────────────────

std::vector<Token> Lexer::tokenise() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();

        if (isAtEnd()) {
            tokens.push_back(makeToken(TokenType::END_OF_FILE, ""));
            break;
        }

        char c = peek();

        if (c == '#') {
            advance(); // consume '#'
            Token comment = scanComment();
            // Discard comments — they never reach the parser
            // Uncomment below to retain them for tooling:
            // tokens.push_back(comment);
            continue;
        }

        if (m_expectImportPath) {
            m_expectImportPath = false;
            tokens.push_back(scanImportPath());
            continue;
        }

        if (std::isdigit(c)) {
            tokens.push_back(scanNumber());
            continue;
        }

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
            continue;
        }

        if (c == '\\') {
            advance(); // consume '\'
            tokens.push_back(scanLatex());
            continue;
        }

        if (c == '"') {
            advance(); // opening quote
            tokens.push_back(scanString());
            continue;
        }

        tokens.push_back(scanSymbol());
    }

    return tokens;
}
