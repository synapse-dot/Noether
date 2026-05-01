// interpreter.cpp

#include "interpreter.hpp"
#include "reconciler.hpp"
#include "simulation_engine.hpp"
#include "renderer/opengl_renderer.hpp"

#include <cmath>
#include <iostream>

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
        default:
            return 0.0;
    }
}

Interpreter::Interpreter(const ProgramNode& program) : m_program(program) {}

void Interpreter::run() {
    Reconciler reconciler(m_program);
    reconciler.reconcile();

    for (const auto& sim : m_program.simulations) {
        simulate(sim);
    }
}

void Interpreter::simulate(const SimulateNode& sim) {
    const SystemNode* sysNode = nullptr;
    for (const auto& s : m_program.systems) {
        if (s.name == sim.systemName) {
            sysNode = &s;
            break;
        }
    }
    if (!sysNode) return;

    Reconciler rec(m_program);
    rec.reconcile();
    const auto& eq = rec.getEquations(sim.systemName);

    std::map<std::string, double> env;
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

    for (const auto& ic : sim.initial) {
        env[ic.name] = m_evaluator.evaluate(*ic.value, {});
        env[ic.name + "_dot"] = 0.0;
    }

    const double duration = m_evaluator.evaluate(*sim.duration.magnitude, {});
    const double dt = m_evaluator.evaluate(*sim.dt.magnitude, {});

    SimulationEngine engine(eq, m_evaluator, std::move(env), dt);

    SceneConfig sceneConfig;
    sceneConfig.durationSeconds = duration;
    sceneConfig.xWorldHalfExtent = 2.0;

    ::run(sceneConfig, engine);
}
