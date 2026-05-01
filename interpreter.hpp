// interpreter.hpp

#pragma once
#include "ast.hpp"
#include <map>
#include <string>
#include <vector>

class Evaluator {
public:
    double evaluate(const ExprNode& expr, const std::map<std::string, double>& env) const;
};

class Interpreter {
public:
    explicit Interpreter(const ProgramNode& program);
    void run();

private:
    const ProgramNode& m_program;
    Evaluator m_evaluator;
    
    // State of all systems
    struct SystemState {
        std::map<std::string, double> variables;
    };
    std::map<std::string, SystemState> m_systemStates;

    void simulate(const SimulateNode& node);
};
