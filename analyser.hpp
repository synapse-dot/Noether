// analyser.hpp

#pragma once
#include "ast.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <vector>

// ── Analysis error ────────────────────────────────────────────────────────────

struct AnalysisError : public std::runtime_error {
    int line;
    int col;
    AnalysisError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

// ── Symbol — what we know about a declared name ───────────────────────────────

struct Symbol {
    std::string name;
    std::string type;    // "int", "real", or "dimensioned"
    DimType     dim;     // only meaningful if type == "dimensioned"
    bool        isPhysical = false; // true if inferred from physical block
    int         line = 0;
};

// ── Symbol table — scoped name → symbol mapping ───────────────────────────────

class SymbolTable {
public:
    void     define(const Symbol& sym);
    bool     isDefined(const std::string& name) const;
    Symbol   lookup(const std::string& name) const;
    std::unordered_set<std::string> names() const;
    void     dump() const; // for debugging

private:
    std::unordered_map<std::string, Symbol> m_symbols;
};

// ── Analyser ──────────────────────────────────────────────────────────────────

class Analyser {
public:
    explicit Analyser(const ProgramNode& program);
    void analyse(); // throws AnalysisError on failure

private:
    const ProgramNode& m_program;

    // Known system names — populated during first pass
    std::unordered_set<std::string> m_systemNames;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_systemSymbols;

    // ── Top-level checks ──────────────────────────────────────────
    void checkSystem(const SystemNode& node);
    void checkSimulate(const SimulateNode& node);
    void checkRender(const RenderNode& node);
    void checkSystemSimulate(const std::string& systemName, const SimulateBlockNode& node);
    void checkSystemVisualize(const std::string& systemName, const VisualizeBlockNode& node);
    void checkPositiveTimeValue(const DimValue& value,
                                const std::string& fieldName,
                                int line) const;
    void checkTimeDimension(const DimValue& value,
                            const std::string& fieldName,
                            int line) const;

    // ── Physical layer checks ─────────────────────────────────────
    void populatePhysicalSymbols(const PhysicalNode& node,
                                  SymbolTable& table);

    // ── Math layer checks ─────────────────────────────────────────
    void checkMath(const MathNode& node, SymbolTable& table);
    void checkDecl(const DeclNode& node, SymbolTable& table);
    void checkAssign(const AssignNode& node, SymbolTable& table);

    // ── Expression checks ─────────────────────────────────────────
    // Returns the inferred DimType of the expression
    DimType checkExpr(const ExprNode& expr,
                      const SymbolTable& table) const;

    // ── Dimensional arithmetic ────────────────────────────────────
    DimType multiplyDims(const DimType& a, const DimType& b) const;
    DimType divideDims(const DimType& a, const DimType& b) const;
    DimType negateDim(const DimType& a) const;
    bool    dimsEqual(const DimType& a, const DimType& b) const;

    // ── Known physical quantities inferred from physical block ────
    // e.g. rod r1 with mass m1, length l1
    void inferRodSymbols(const RodNode& rod, SymbolTable& table);
    void inferParticleSymbols(const ParticleNode& p, SymbolTable& table);
    void inferSpringSymbols(const SpringNode& s, SymbolTable& table);
    void inferGravitySymbol(const GravityNode& g, SymbolTable& table);

    // ── Built-in functions ────────────────────────────────────────
    // Returns the output dimension given the function name and arg dims
    DimType checkBuiltinCall(const std::string& name,
                              const std::vector<DimType>& argDims,
                              int line, int col) const;
};
