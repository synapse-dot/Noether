// token.hpp

#pragma once
#include <string>
#include <variant>

enum class TokenType {

    // ── Literals ────────────────────────────────────────────────
    LIT_INT,        // 42, 7
    LIT_REAL,       // 3.14, 9.8
    LIT_SCI,        // 1.0e3, 9.8e0 (post-normalisation)

    // ── Dimension symbols ────────────────────────────────────────
    DIM_L,          // L
    DIM_M,          // M
    DIM_T,          // T
    DIM_I,          // I
    DIM_K,          // K
    DIM_N,          // N
    DIM_J,          // J

    // ── Keywords — structure ─────────────────────────────────────
    KW_SYSTEM,      // system
    KW_PHYSICAL,    // physical
    KW_MATH,        // math
    KW_SIMULATE,    // simulate
    KW_RENDER,      // render
    KW_IMPORT,      // import
    KW_LET,         // let

    // ── Keywords — physical primitives ───────────────────────────
    KW_ROD,         // rod
    KW_PARTICLE,    // particle
    KW_SPRING,      // spring
    KW_GRAVITY,     // gravity
    KW_ORIGIN,      // origin
    KW_DOWNWARD,    // downward
    KW_UPWARD,      // upward

    // ── Keywords — mathematical ───────────────────────────────────
    KW_LAG,         // lag   (Lagrangian)
    KW_LET_T,       // t     (kinetic energy — reserved)
    KW_LET_V,       // v     (potential energy — reserved)

    // ── Keywords — simulation ────────────────────────────────────
    KW_DURATION,    // duration
    KW_DT,          // dt
    KW_INITIAL,     // initial
    KW_MODE,        // mode
    KW_REALTIME,    // realtime
    KW_EXPORT,      // export

    // ── Keywords — types ─────────────────────────────────────────
    KW_INT,         // int
    KW_REAL,        // real

    // ── LaTeX-inspired operators ──────────────────────────────────
    LATEX_PARTIAL,  // \partial
    LATEX_INTEGRAL, // \integral
    LATEX_SUM,      // \sum
    LATEX_SQRT,     // \sqrt
    LATEX_INF,      // \inf

    // ── Arithmetic operators ──────────────────────────────────────
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_STAR,        // *
    OP_CARET,       // ^
    OP_SLASH,       // /
    OP_EQUALS,      // =

    // ── Delimiters ────────────────────────────────────────────────
    DELIM_LBRACE,   // {
    DELIM_RBRACE,   // }
    DELIM_LBRACKET, // [
    DELIM_RBRACKET, // ]
    DELIM_LPAREN,   // (
    DELIM_RPAREN,   // )
    DELIM_COMMA,    // ,
    DELIM_COLON,    // :
    DELIM_DOT,      // .

    // ── Identifiers & special ─────────────────────────────────────
    IDENTIFIER,     // any lowercase user-defined name
    SYSTEM_NAME,     // any PascalCase system name 
    COMMENT,        // # ...  (may be discarded after lexing)
    NEWLINE,        // may be needed for error reporting
    END_OF_FILE,    // sentinel

    // ── Error ─────────────────────────────────────────────────────
    UNKNOWN         // unrecognised character — lexer error
};

// Every token carries its type, its raw text, and its position for error reporting
struct Token {
    TokenType   type;
    std::string lexeme;   // raw text as written in source
    int         line;
    int         col;

    // For numeric literals, we store the normalised value separately
    std::variant<std::monostate, long long, double> value;
};