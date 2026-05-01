// interpreter.cpp

#include "interpreter.hpp"
#include "reconciler.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>

double Evaluator::evaluate(const ExprNode& expr, const std::map<std::string, double>& env) const {
    switch (expr.kind) {
        case ExprNode::Kind::NUMBER:
            return expr.number;
        case ExprNode::Kind::IDENTIFIER:
            if (env.count(expr.name)) return env.at(expr.name);
            return 0.0;
        case ExprNode::Kind::UNARY:
            if (expr.unaryOp == "-") return -evaluate(*expr.operand, env);
            return 0.0;
        case ExprNode::Kind::BINARY: {
            double l = evaluate(*expr.left, env);
            double r = evaluate(*expr.right, env);
            if (expr.binaryOp == "+") return l + r;
            if (expr.binaryOp == "-") return l - r;
            if (expr.binaryOp == "*") return l * r;
            if (expr.binaryOp == "/") return l / r;
            if (expr.binaryOp == "^") return std::pow(l, r);
            return 0.0;
        }
        case ExprNode::Kind::CALL: {
            double arg = evaluate(*expr.args[0], env);
            if (expr.callee == "cos") return std::cos(arg);
            if (expr.callee == "sin") return std::sin(arg);
            if (expr.callee == "tan") return std::tan(arg);
            if (expr.callee == "sqrt") return std::sqrt(arg);
            return 0.0;
        }
        default: return 0.0;
    }
}

Interpreter::Interpreter(const ProgramNode& program) : m_program(program) {}

void Interpreter::run() {
    // 1. Reconcile all systems
    Reconciler reconciler(m_program);
    reconciler.reconcile();

    // 2. Process simulations
    for (const auto& sim : m_program.simulations) {
        simulate(sim);
    }
}

// Simple Gaussian Elimination for small systems
std::vector<double> solveLinearSystem(std::vector<std::vector<double>>& M, std::vector<double>& F) {
    int n = F.size();
    for (int i = 0; i < n; ++i) {
        // Pivot
        double pivot = M[i][i];
        for (int j = i + 1; j < n; ++j) {
            double factor = M[j][i] / pivot;
            F[j] -= factor * F[i];
            for (int k = i; k < n; ++k) {
                M[j][k] -= factor * M[i][k];
            }
        }
    }
    std::vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0;
        for (int j = i + 1; j < n; ++j) {
            sum += M[i][j] * x[j];
        }
        x[i] = (F[i] - sum) / M[i][i];
    }
    return x;
}

void Interpreter::simulate(const SimulateNode& sim) {
    std::cout << "\n--- Starting Simulation: " << sim.systemName << " ---\n";

    // Find the system
    const SystemNode* sysNode = nullptr;
    for (const auto& s : m_program.systems) {
        if (s.name == sim.systemName) {
            sysNode = &s;
            break;
        }
    }
    if (!sysNode) return;

    // Get derived equations
    Reconciler rec(m_program);
    rec.reconcile();
    const auto& eq = rec.getEquations(sim.systemName);

    // Initial state
    std::map<std::string, double> env;
    
    // Set constants from physical block
    if (sysNode->physical) {
        for (const auto& rod : sysNode->physical->rods) {
            std::string idx = rod.name.substr(1);
            env["m" + idx] = m_evaluator.evaluate(*rod.mass.magnitude, {});
            env["l" + idx] = m_evaluator.evaluate(*rod.length.magnitude, {});
        }
        if (sysNode->physical->gravity) {
            env["g"] = m_evaluator.evaluate(*sysNode->physical->gravity->magnitude.magnitude, {});
        }
    }

    // Set initial conditions
    for (const auto& ic : sim.initial) {
        env[ic.name] = m_evaluator.evaluate(*ic.value, {});
        env[ic.name + "_dot"] = 0.0; 
    }

    double duration = m_evaluator.evaluate(*sim.duration.magnitude, {});
    double dt = m_evaluator.evaluate(*sim.dt.magnitude, {});
    double t = 0.0;

    std::cout << "Time      ";
    for (const auto& q : eq.coordinates) std::cout << std::setw(12) << q;
    std::cout << "\n------------------------------------------------\n";

    while (t <= duration) {
        // Print state
        std::cout << std::fixed << std::setprecision(2) << t << "      ";
        for (const auto& q : eq.coordinates) std::cout << std::setw(12) << env[q];
        std::cout << "\n";

        // 1. Evaluate M and F
        int n = eq.coordinates.size();
        std::vector<std::vector<double>> M(n, std::vector<double>(n));
        std::vector<double> F(n);

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                M[i][j] = m_evaluator.evaluate(*eq.massMatrix[i][j], env);
            }
            F[i] = m_evaluator.evaluate(*eq.forceVector[i], env);
        }

        // 2. Solve for q_ddot
        std::vector<double> q_ddot = solveLinearSystem(M, F);

        // 3. Update state (Euler Integration)
        for (int i = 0; i < n; ++i) {
            std::string q = eq.coordinates[i];
            std::string q_dot = q + "_dot";

            env[q] += env[q_dot] * dt;
            env[q_dot] += q_ddot[i] * dt;
        }

        t += dt;
    }
}
