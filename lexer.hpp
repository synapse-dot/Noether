// lexer.hpp

#pragma once
#include "token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenise();

private:
    std::string m_source;
    std::size_t m_pos;
    int         m_line;
    int         m_col;
    bool        m_expectSystemName = false; // ← this was missing

    // ── Character navigation ──────────────────────────────────────
    char        peek() const;
    char        peekNext() const;
    char        advance();
    bool        isAtEnd() const;

    // ── Token producers ───────────────────────────────────────────
    Token       makeToken(TokenType type, const std::string& lexeme) const;
    Token       errorToken(const std::string& msg) const;

    // ── Scanning helpers ──────────────────────────────────────────
    void        skipWhitespace();
    Token       scanComment();
    Token       scanNumber();
    Token       scanIdentifierOrKeyword();
    Token       scanLatex();
    Token       scanDimension(char c);
    Token       scanSymbol();
};