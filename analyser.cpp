// analyser.cpp

#include "analyser.hpp"
#include <iostream>
#include <string>

// ── SymbolTable ───────────────────────────────────────────────────────────────

void SymbolTable::define(const Symbol& sym) {
    m_symbols[sym.name] = sym;
}

bool SymbolTable::isDefined(const std::string& name) const {
    return m_symbols.count(name) > 0;
}

Symbol SymbolTable::lookup(const std::string& name) const {
    return m_symbols.at(name);
}

std::unordered_set<std::string> SymbolTable::names() const {
    std::unordered_set<std::string> out;
    out.reserve(m_symbols.size());
    for (const auto& [name, _] : m_symbols)
        out.insert(name);
    return out;
}

void SymbolTable::dump() const {
    for (const auto& [name, sym] : m_symbols)
        std::cout << "  " << name
                  << " : " << sym.type
                  << (sym.isPhysical ? " [physical]" : "")
                  << "\n";
}

// ── Constructor ───────────────────────────────────────────────────────────────

Analyser::Analyser(const ProgramNode& program)
    : m_program(program) {}

// ── Top level ─────────────────────────────────────────────────────────────────

void Analyser::analyse() {

    // First pass — collect all system names so simulate/render
    // can reference them
    for (const auto& sys : m_program.systems)
        m_systemNames.insert(sys.name);

    // Second pass — check each system
    for (const auto& sys : m_program.systems)
        checkSystem(sys);

    // Check simulate blocks
    for (const auto& sim : m_program.simulations)
        checkSimulate(sim);

    // Check render blocks
    for (const auto& ren : m_program.renders)
        checkRender(ren);
}

// ── System checking ───────────────────────────────────────────────────────────

void Analyser::checkSystem(const SystemNode& node) {

    // Both physical and math blocks are mandatory — Reconciler is strict
    if (!node.physical)
        throw AnalysisError(
            "System '" + node.name + "' is missing a physical block",
            node.line, 0);

    if (!node.math)
        throw AnalysisError(
            "System '" + node.name + "' is missing a math block",
            node.line, 0);

    // Build symbol table from physical block first
    SymbolTable table;
    populatePhysicalSymbols(*node.physical, table);

    // Then check math block against it
    checkMath(*node.math, table);

    m_systemSymbols[node.name] = table.names();

    if (node.simulate)
        checkSystemSimulate(node.name, *node.simulate);

    if (node.visualize)
        checkSystemVisualize(node.name, *node.visualize);
}

// ── Physical symbol population ────────────────────────────────────────────────

void Analyser::populatePhysicalSymbols(const PhysicalNode& node,
                                        SymbolTable& table) {
    for (const auto& rod      : node.rods)      inferRodSymbols(rod, table);
    for (const auto& particle : node.particles) inferParticleSymbols(particle, table);
    for (const auto& spring   : node.springs)   inferSpringSymbols(spring, table);
    if  (node.gravity)                          inferGravitySymbol(*node.gravity, table);
}

// ── Physical inference ────────────────────────────────────────────────────────
// Each physical primitive implies a set of named quantities in the math layer.
// A rod named r1 implies: m1 [M], l1 [L], theta1 [dimensionless] by convention.
// We infer the index from the rod's name suffix.

static std::string suffix(const std::string& name) {
    // Extract trailing digits or letter — r1 → "1", r2 → "2"
    std::string s;
    for (char c : name)
        if (std::isdigit(c) || std::isalpha(c)) s = std::string(1, c);
    // Just return everything after the first character
    return name.size() > 1 ? name.substr(1) : name;
}

void Analyser::inferRodSymbols(const RodNode& rod, SymbolTable& table) {
    std::string idx = suffix(rod.name); // "1" from "r1"

    // mass: m1 [M]
    Symbol mass;
    mass.name       = "m" + idx;
    mass.type       = "dimensioned";
    mass.dim        = rod.mass.dim;
    mass.isPhysical = true;
    mass.line       = rod.line;
    table.define(mass);

    // length: l1 [L]
    Symbol len;
    len.name       = "l" + idx;
    len.type       = "dimensioned";
    len.dim        = rod.length.dim;
    len.isPhysical = true;
    len.line       = rod.line;
    table.define(len);
}

void Analyser::inferParticleSymbols(const ParticleNode& p,
                                     SymbolTable& table) {
    std::string idx = suffix(p.name);

    Symbol mass;
    mass.name       = "m" + idx;
    mass.type       = "dimensioned";
    mass.dim        = p.mass.dim;
    mass.isPhysical = true;
    mass.line       = p.line;
    table.define(mass);
}

void Analyser::inferSpringSymbols(const SpringNode& s, SymbolTable& table) {
    std::string idx = suffix(s.name);

    Symbol k;
    k.name       = "k" + idx;
    k.type       = "dimensioned";
    k.dim        = s.stiffness.dim;
    k.isPhysical = true;
    k.line       = s.line;
    table.define(k);
}

void Analyser::inferGravitySymbol(const GravityNode& g, SymbolTable& table) {
    // gravity is always available as 'g' in the math layer
    Symbol sym;
    sym.name       = "g";
    sym.type       = "dimensioned";
    sym.dim        = g.magnitude.dim;
    sym.isPhysical = true;
    sym.line       = g.line;
    table.define(sym);
}

// ── Math checking ─────────────────────────────────────────────────────────────

void Analyser::checkMath(const MathNode& node, SymbolTable& table) {
    // Process declarations first so variables are in scope for assignments
    for (const auto& decl : node.decls)
        checkDecl(decl, table);

    // Then check assignments
    for (const auto& assign : node.assignments) {
        checkAssign(assign, table);
        
        // If we just assigned 'lag', check its dimensions
        if (assign.target == "lag") {
            Symbol lagSym = table.lookup("lag");
            DimType energyDim;
            energyDim.M = 1; energyDim.L = 2; energyDim.T = -2;
            if (!dimsEqual(lagSym.dim, energyDim)) {
                throw AnalysisError(
                    "Lagrangian 'lag' must have dimensions of Energy [M*L^2*T^-2]",
                    assign.line, 0);
            }
        }
    }
}

void Analyser::checkDecl(const DeclNode& node, SymbolTable& table) {
    for (const auto& var : node.vars) {
        if (table.isDefined(var.name))
            throw AnalysisError(
                "Variable '" + var.name + "' is already defined",
                var.line, 0);

        Symbol sym;
        sym.name = var.name;
        sym.type = var.type;
        sym.line = var.line;
        table.define(sym);

        // Also define _dot and _ddot for physical variables
        Symbol dot;
        dot.name = var.name + "_dot";
        dot.type = "dimensioned";
        dot.dim  = sym.dim;
        dot.dim.T -= 1;
        dot.line = var.line;
        table.define(dot);

        Symbol ddot;
        ddot.name = var.name + "_ddot";
        ddot.type = "dimensioned";
        ddot.dim  = sym.dim;
        ddot.dim.T -= 2;
        ddot.line = var.line;
        table.define(ddot);
    }
}

void Analyser::checkAssign(const AssignNode& node, SymbolTable& table) {
    if (!node.expr) return;

    // Check the expression — this validates all identifiers within it
    DimType resultDim = checkExpr(*node.expr, table);

    // Register the assignment target in the symbol table
    // so it can be referenced later (e.g. lag = t - v)
    if (!table.isDefined(node.target)) {
        Symbol sym;
        sym.name = node.target;
        sym.type = resultDim.isDimensionless() ? "real" : "dimensioned";
        sym.dim  = resultDim;
        sym.line = node.line;
        table.define(sym);
    }
}

// ── Expression checking ───────────────────────────────────────────────────────

DimType Analyser::checkExpr(const ExprNode& expr,
                              const SymbolTable& table) const {
    switch (expr.kind) {

        case ExprNode::Kind::NUMBER:
            // Plain numbers are dimensionless
            return DimType{};

        case ExprNode::Kind::IDENTIFIER: {
            if (!table.isDefined(expr.name)) {
                // Remote fallback: allow conventional time-derivative identifiers
                auto hasSuffix = [](const std::string& value,
                                    const std::string& suffix) {
                    return value.size() >= suffix.size() &&
                           value.compare(value.size() - suffix.size(),
                                         suffix.size(), suffix) == 0;
                };

                std::string base = expr.name;
                int dotCount = 0;
                while (hasSuffix(base, "_dot")) {
                    base = base.substr(0, base.size() - 4);
                    ++dotCount;
                }

                if (base != expr.name && table.isDefined(base)) {
                    Symbol sym = table.lookup(base);
                    DimType derived = sym.dim;
                    derived.T -= dotCount;
                    return derived;
                }

                throw AnalysisError(
                    "Undefined variable '" + expr.name + "'",
                    expr.line, expr.col);
            }
            Symbol sym = table.lookup(expr.name);
            return sym.dim;
        }

        case ExprNode::Kind::UNARY: {
            DimType operandDim = checkExpr(*expr.operand, table);
            return operandDim; // negation preserves dimension
        }

        case ExprNode::Kind::BINARY: {
            DimType leftDim  = checkExpr(*expr.left,  table);
            DimType rightDim = checkExpr(*expr.right, table);

            if (expr.binaryOp == "+" || expr.binaryOp == "-") {
                // Addition and subtraction require matching dimensions
                if (!dimsEqual(leftDim, rightDim))
                    throw AnalysisError(
                        "Dimension mismatch in '" + expr.binaryOp
                        + "' expression",
                        expr.line, expr.col);
                return leftDim;
            }

            if (expr.binaryOp == "*")
                return multiplyDims(leftDim, rightDim);

            if (expr.binaryOp == "/")
                return divideDims(leftDim, rightDim);

            if (expr.binaryOp == "^") {
                // Exponentiation — right side must be dimensionless
                if (!rightDim.isDimensionless())
                    throw AnalysisError(
                        "Exponent must be dimensionless",
                        expr.line, expr.col);

                // If exponent is a literal number, we can infer the new dimension
                if (expr.right->kind == ExprNode::Kind::NUMBER) {
                    double p = expr.right->number;
                    return DimType{
                        static_cast<int>(leftDim.L * p),
                        static_cast<int>(leftDim.M * p),
                        static_cast<int>(leftDim.T * p),
                        static_cast<int>(leftDim.I * p),
                        static_cast<int>(leftDim.K * p),
                        static_cast<int>(leftDim.N * p),
                        static_cast<int>(leftDim.J * p)
                    };
                }
                return leftDim;
            }

            return DimType{};
        }

        case ExprNode::Kind::CALL: {
            // Check all arguments and collect their dimensions
            std::vector<DimType> argDims;
            for (const auto& arg : expr.args)
                argDims.push_back(checkExpr(*arg, table));

            return checkBuiltinCall(expr.callee, argDims,
                                    expr.line, expr.col);
        }

        case ExprNode::Kind::LATEX:
            // LaTeX operators — dimension inference deferred
            return DimType{};
    }

    return DimType{};
}

// ── Dimensional arithmetic ────────────────────────────────────────────────────

DimType Analyser::multiplyDims(const DimType& a, const DimType& b) const {
    return DimType{
        a.L + b.L, a.M + b.M, a.T + b.T,
        a.I + b.I, a.K + b.K, a.N + b.N, a.J + b.J
    };
}

DimType Analyser::divideDims(const DimType& a, const DimType& b) const {
    return DimType{
        a.L - b.L, a.M - b.M, a.T - b.T,
        a.I - b.I, a.K - b.K, a.N - b.N, a.J - b.J
    };
}

bool Analyser::dimsEqual(const DimType& a, const DimType& b) const {
    return a.L == b.L && a.M == b.M && a.T == b.T &&
           a.I == b.I && a.K == b.K && a.N == b.N && a.J == b.J;
}

// ── Built-in functions ────────────────────────────────────────────────────────

DimType Analyser::checkBuiltinCall(const std::string& name,
                                    const std::vector<DimType>& argDims,
                                    int line, int col) const {
    // Trigonometric functions — argument must be dimensionless,
    // result is dimensionless
    if (name == "cos" || name == "sin" || name == "tan" ||
        name == "acos"|| name == "asin"|| name == "atan") {

        if (argDims.size() != 1)
            throw AnalysisError(
                "'" + name + "' expects exactly one argument",
                line, col);

        if (!argDims[0].isDimensionless())
            throw AnalysisError(
                "Argument to '" + name + "' must be dimensionless "
                "(angles are dimensionless in Noether)",
                line, col);

        return DimType{}; // dimensionless result
    }

    // sqrt — argument dimensionless for now
    if (name == "sqrt") {
        if (argDims.size() != 1)
            throw AnalysisError(
                "'sqrt' expects exactly one argument", line, col);
        return DimType{};
    }

    // Unknown function — warn but don't hard-error,
    // as it may be from an imported library
    return DimType{};
}

// ── Simulate checking ─────────────────────────────────────────────────────────

void Analyser::checkSimulate(const SimulateNode& node) {
    if (!m_systemNames.count(node.systemName))
        throw AnalysisError(
            "simulate references undefined system '"
            + node.systemName + "'",
            node.line, 0);

    checkPositiveTimeValue(node.duration, "duration", node.line);
    checkTimeDimension(node.duration, "duration", node.line);

    checkPositiveTimeValue(node.dt, "dt", node.line);
    checkTimeDimension(node.dt, "dt", node.line);
}

// ── Render checking ───────────────────────────────────────────────────────────

void Analyser::checkRender(const RenderNode& node) {
    if (!m_systemNames.count(node.systemName))
        throw AnalysisError(
            "render references undefined system '"
            + node.systemName + "'",
            node.line, 0);
}

void Analyser::checkSystemSimulate(const std::string& systemName,
                                   const SimulateBlockNode& node) {
    if (node.integrator != "rk4" && node.integrator != "euler")
        throw AnalysisError(
            "system '" + systemName + "' has unsupported integrator '"
            + node.integrator + "' (expected rk4 or euler)",
            node.line, 0);

    checkPositiveTimeValue(node.timestep, "timestep", node.line);
    checkTimeDimension(node.timestep, "timestep", node.line);

    checkPositiveTimeValue(node.duration, "duration", node.line);
    checkTimeDimension(node.duration, "duration", node.line);
}

void Analyser::checkSystemVisualize(const std::string& systemName,
                                    const VisualizeBlockNode& node) {
    const auto it = m_systemSymbols.find(systemName);
    if (it == m_systemSymbols.end()) return;

    const auto& symbols = it->second;
    for (const auto& plot : node.plots) {
        if (!symbols.count(plot.quantity))
            throw AnalysisError(
                "plot references undefined symbol '" + plot.quantity + "'",
                plot.line, 0);
        if (!symbols.count(plot.against))
            throw AnalysisError(
                "plot references undefined symbol '" + plot.against + "'",
                plot.line, 0);
    }
}

void Analyser::checkPositiveTimeValue(const DimValue& value,
                                      const std::string& fieldName,
                                      int line) const {
    if (!value.magnitude) return;
    if (value.magnitude->kind == ExprNode::Kind::NUMBER && value.magnitude->number <= 0.0)
        throw AnalysisError(fieldName + " must be > 0", line, 0);
}

void Analyser::checkTimeDimension(const DimValue& value,
                                  const std::string& fieldName,
                                  int line) const {
    DimType expected;
    expected.T = 1;
    if (!dimsEqual(value.dim, expected))
        throw AnalysisError(fieldName + " must have time dimension [T]", line, 0);
}
