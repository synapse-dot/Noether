#include "simulation_engine.hpp"
#include "interpreter.hpp"

#include <cmath>
#include <vector>

namespace {
std::vector<double> solveLinearSystem(std::vector<std::vector<double>>& matrix, std::vector<double>& force) {
    const int n = static_cast<int>(force.size());
    for (int i = 0; i < n; ++i) {
        const double pivot = matrix[i][i];
        for (int j = i + 1; j < n; ++j) {
            const double factor = matrix[j][i] / pivot;
            force[j] -= factor * force[i];
            for (int k = i; k < n; ++k) {
                matrix[j][k] -= factor * matrix[i][k];
            }
        }
    }

    std::vector<double> solution(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        for (int j = i + 1; j < n; ++j) {
            sum += matrix[i][j] * solution[j];
        }
        solution[i] = (force[i] - sum) / matrix[i][i];
    }
    return solution;
}
} // namespace

SimulationEngine::SimulationEngine(const SystemEquations& equations,
                                   const Evaluator& evaluator,
                                   std::map<std::string, double> initialEnvironment,
                                   double dt)
    : m_equations(equations), m_evaluator(evaluator), m_env(std::move(initialEnvironment)), m_dt(dt) {}

FrameState SimulationEngine::currentFrame() const {
    return FrameState{m_time, computeRenderableQuantities()};
}

FrameState SimulationEngine::step() {
    const int n = static_cast<int>(m_equations.coordinates.size());
    std::vector<std::vector<double>> massMatrix(n, std::vector<double>(n));
    std::vector<double> forceVector(n);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            massMatrix[i][j] = m_evaluator.evaluate(*m_equations.massMatrix[i][j], m_env);
        }
        forceVector[i] = m_evaluator.evaluate(*m_equations.forceVector[i], m_env);
    }

    const std::vector<double> qddot = solveLinearSystem(massMatrix, forceVector);

    for (int i = 0; i < n; ++i) {
        const std::string& q = m_equations.coordinates[i];
        const std::string qDot = q + "_dot";

        m_env[q] += m_env[qDot] * m_dt;
        m_env[qDot] += qddot[i] * m_dt;
    }

    m_time += m_dt;
    return currentFrame();
}

std::map<std::string, double> SimulationEngine::computeRenderableQuantities() const {
    std::map<std::string, double> out = m_env;

    if (m_env.count("theta1") && m_env.count("l1")) {
        const double theta1 = m_env.at("theta1");
        const double l1 = m_env.at("l1");
        const double x1 = l1 * std::sin(theta1);
        const double y1 = l1 * std::cos(theta1);

        out["x"] = x1;
        out["y"] = y1;
        out["x1"] = x1;
        out["y1"] = y1;

        if (m_env.count("theta2") && m_env.count("l2")) {
            const double theta2 = m_env.at("theta2");
            const double l2 = m_env.at("l2");
            out["x2"] = x1 + l2 * std::sin(theta2);
            out["y2"] = y1 + l2 * std::cos(theta2);
        }
    }

    return out;
}
