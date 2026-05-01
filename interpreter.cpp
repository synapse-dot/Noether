// interpreter.cpp

#include "interpreter.hpp"
#include "reconciler.hpp"
#include "simulation_engine.hpp"

#include <cmath>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

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

void renderASCII(const FrameState& frame) {
    const int WIDTH = 60;
    const int HEIGHT = 30;
    const int OFFSET_X = WIDTH / 2;
    const int OFFSET_Y = 10;
    const double SCALE = 8.0;

    std::vector<std::string> screen(HEIGHT, std::string(WIDTH, ' '));

    const auto q = frame.quantities;
    const int x1 = static_cast<int>(OFFSET_X + q.at("x1") * SCALE);
    const int y1 = static_cast<int>(OFFSET_Y + q.at("y1") * SCALE);

    int x2 = x1;
    int y2 = y1;
    if (q.count("x2") && q.count("y2")) {
        x2 = static_cast<int>(OFFSET_X + q.at("x2") * SCALE);
        y2 = static_cast<int>(OFFSET_Y + q.at("y2") * SCALE);
    }

    screen[OFFSET_Y][OFFSET_X] = 'O';

    auto drawLine = [&](int x0, int y0, int x1p, int y1p, char c) {
        int dx = std::abs(x1p - x0), sx = x0 < x1p ? 1 : -1;
        int dy = -std::abs(y1p - y0), sy = y0 < y1p ? 1 : -1;
        int err = dx + dy;
        while (true) {
            if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) screen[y0][x0] = c;
            if (x0 == x1p && y0 == y1p) break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    };

    drawLine(OFFSET_X, OFFSET_Y, x1, y1, '.');
    drawLine(x1, y1, x2, y2, ':');

    if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) screen[y1][x1] = '@';
    if (x2 >= 0 && x2 < WIDTH && y2 >= 0 && y2 < HEIGHT) screen[y2][x2] = '#';

    std::cout << "\033[H\033[J";
    std::cout << "Noether v1.0 — ASCII Renderer | Time: " << std::fixed << std::setprecision(2)
              << frame.time << "s\n";
    for (const auto& row : screen) std::cout << row << "\n";
    std::cout << "Legend: O (Origin), @ (Mass 1), # (Mass 2)\n";
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

    for (double t = 0.0; t <= duration; t += dt) {
        const FrameState frame = engine.currentFrame();
        renderASCII(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
        engine.step();
    }
}
