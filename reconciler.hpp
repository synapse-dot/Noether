// reconciler.hpp

#pragma once
#include "ast.hpp"
#include <vector>
#include <string>
#include <map>

struct SystemEquations {
    std::vector<std::string> coordinates;
    // M_ij expressions (mass matrix)
    std::vector<std::vector<std::unique_ptr<ExprNode>>> massMatrix;
    // F_i expressions (force vector)
    std::vector<std::unique_ptr<ExprNode>> forceVector;
};

class Reconciler {
public:
    explicit Reconciler(const ProgramNode& program);
    void reconcile();
    
    // Get the derived equations for a system
    const SystemEquations& getEquations(const std::string& systemName) const;

private:
    const ProgramNode& m_program;
    std::map<std::string, SystemEquations> m_systemEquations;

    void processSystem(const SystemNode& sys);
    
    // Symbolic differentiation
    std::unique_ptr<ExprNode> diff(const ExprNode& expr, const std::string& var);
    std::unique_ptr<ExprNode> simplify(std::unique_ptr<ExprNode> expr);
    
    // Helper to clone expressions
    std::unique_ptr<ExprNode> clone(const ExprNode& expr);
};
