#pragma once

#include "frame_state.hpp"
#include "reconciler.hpp"

#include <map>
#include <string>

class Evaluator;

class SimulationEngine {
public:
    SimulationEngine(const SystemEquations& equations,
                     const Evaluator& evaluator,
                     std::map<std::string, double> initialEnvironment,
                     double dt);

    FrameState currentFrame() const;
    FrameState step();

private:
    const SystemEquations& m_equations;
    const Evaluator& m_evaluator;
    std::map<std::string, double> m_env;
    double m_dt;
    double m_time = 0.0;

    std::map<std::string, double> computeRenderableQuantities() const;
};
