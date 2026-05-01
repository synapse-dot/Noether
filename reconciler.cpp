// reconciler.cpp

#include "reconciler.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

Reconciler::Reconciler(const ProgramNode& program) : m_program(program) {}

void Reconciler::reconcile() {
    for (const auto& sys : m_program.systems) {
        processSystem(sys);
    }
}

const SystemEquations& Reconciler::getEquations(const std::string& systemName) const {
    return m_systemEquations.at(systemName);
}

void Reconciler::processSystem(const SystemNode& sys) {
    SystemEquations eq;
    
    // 1. Collect coordinates
    if (sys.math) {
        for (const auto& decl : sys.math->decls) {
            for (const auto& var : decl.vars) {
                eq.coordinates.push_back(var.name);
            }
        }
    }

    // 2. Find Lagrangian and expand definitions
    std::map<std::string, const ExprNode*> definitions;
    if (sys.math) {
        for (const auto& assign : sys.math->assignments) {
            definitions[assign.target] = assign.expr.get();
        }
    }

    auto expand = [&](auto self, const ExprNode& expr) -> std::unique_ptr<ExprNode> {
        if (expr.kind == ExprNode::Kind::IDENTIFIER && definitions.count(expr.name)) {
            return self(self, *definitions.at(expr.name));
        }
        auto n = std::make_unique<ExprNode>();
        n->kind = expr.kind;
        n->number = expr.number;
        n->name = expr.name;
        n->unaryOp = expr.unaryOp;
        if (expr.operand) n->operand = self(self, *expr.operand);
        n->binaryOp = expr.binaryOp;
        if (expr.left) n->left = self(self, *expr.left);
        if (expr.right) n->right = self(self, *expr.right);
        n->callee = expr.callee;
        for (const auto& arg : expr.args) n->args.push_back(self(self, *arg));
        n->latexOp = expr.latexOp;
        return n;
    };

    std::unique_ptr<ExprNode> lag_expanded;
    if (definitions.count("lag")) {
        lag_expanded = expand(expand, *definitions.at("lag"));
    } else if (definitions.count("t") && definitions.count("v")) {
        auto node = std::make_unique<ExprNode>();
        node->kind = ExprNode::Kind::BINARY;
        node->binaryOp = "-";
        node->left = expand(expand, *definitions.at("t"));
        node->right = expand(expand, *definitions.at("v"));
        lag_expanded = std::move(node);
    }

    if (!lag_expanded) return;
    const ExprNode* lag = lag_expanded.get();

    // 3. Derive M_ij and F_i
    int n = eq.coordinates.size();
    eq.massMatrix.resize(n);
    eq.forceVector.resize(n);

    for (int i = 0; i < n; ++i) {
        std::string qi = eq.coordinates[i];
        std::string qi_dot = qi + "_dot";

        // dL/dqi_dot
        auto dL_dqi_dot = diff(*lag, qi_dot);

        // M_ij = d/dqj_dot (dL/dqi_dot)
        for (int j = 0; j < n; ++j) {
            std::string qj_dot = eq.coordinates[j] + "_dot";
            eq.massMatrix[i].push_back(diff(*dL_dqi_dot, qj_dot));
        }

        // F_i = dL/dqi - sum_j (d/dqj (dL/dqi_dot)) * qj_dot
        auto dL_dqi = diff(*lag, qi);
        
        auto sum_term = std::make_unique<ExprNode>();
        sum_term->kind = ExprNode::Kind::NUMBER;
        sum_term->number = 0.0;

        for (int j = 0; j < n; ++j) {
            std::string qj = eq.coordinates[j];
            std::string qj_dot = qj + "_dot";

            auto d2L_dqi_dot_dqj = diff(*dL_dqi_dot, qj);
            
            auto term = std::make_unique<ExprNode>();
            term->kind = ExprNode::Kind::BINARY;
            term->binaryOp = "*";
            term->left = std::move(d2L_dqi_dot_dqj);
            auto qj_dot_node = std::make_unique<ExprNode>();
            qj_dot_node->kind = ExprNode::Kind::IDENTIFIER;
            qj_dot_node->name = qj_dot;
            term->right = std::move(qj_dot_node);

            auto new_sum = std::make_unique<ExprNode>();
            new_sum->kind = ExprNode::Kind::BINARY;
            new_sum->binaryOp = "+";
            new_sum->left = std::move(sum_term);
            new_sum->right = std::move(term);
            sum_term = simplify(std::move(new_sum));
        }

        auto fi = std::make_unique<ExprNode>();
        fi->kind = ExprNode::Kind::BINARY;
        fi->binaryOp = "-";
        fi->left = std::move(dL_dqi);
        fi->right = std::move(sum_term);
        eq.forceVector[i] = simplify(std::move(fi));
    }

    m_systemEquations[sys.name] = std::move(eq);
}

std::unique_ptr<ExprNode> Reconciler::clone(const ExprNode& expr) {
    auto n = std::make_unique<ExprNode>();
    n->kind = expr.kind;
    n->number = expr.number;
    n->name = expr.name;
    n->unaryOp = expr.unaryOp;
    if (expr.operand) n->operand = clone(*expr.operand);
    n->binaryOp = expr.binaryOp;
    if (expr.left) n->left = clone(*expr.left);
    if (expr.right) n->right = clone(*expr.right);
    n->callee = expr.callee;
    for (const auto& arg : expr.args) n->args.push_back(clone(*arg));
    n->latexOp = expr.latexOp;
    n->line = expr.line;
    n->col = expr.col;
    return n;
}

std::unique_ptr<ExprNode> Reconciler::diff(const ExprNode& expr, const std::string& var) {
    auto result = std::make_unique<ExprNode>();
    result->line = expr.line;
    result->col = expr.col;

    switch (expr.kind) {
        case ExprNode::Kind::NUMBER:
            result->kind = ExprNode::Kind::NUMBER;
            result->number = 0.0;
            break;

        case ExprNode::Kind::IDENTIFIER:
            result->kind = ExprNode::Kind::NUMBER;
            result->number = (expr.name == var) ? 1.0 : 0.0;
            break;

        case ExprNode::Kind::UNARY:
            if (expr.unaryOp == "-") {
                result->kind = ExprNode::Kind::UNARY;
                result->unaryOp = "-";
                result->operand = diff(*expr.operand, var);
            }
            break;

        case ExprNode::Kind::BINARY: {
            if (expr.binaryOp == "+") {
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "+";
                result->left = diff(*expr.left, var);
                result->right = diff(*expr.right, var);
            } else if (expr.binaryOp == "-") {
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "-";
                result->left = diff(*expr.left, var);
                result->right = diff(*expr.right, var);
            } else if (expr.binaryOp == "*") {
                // Product rule: (f*g)' = f'g + fg'
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "+";
                
                auto leftTerm = std::make_unique<ExprNode>();
                leftTerm->kind = ExprNode::Kind::BINARY;
                leftTerm->binaryOp = "*";
                leftTerm->left = diff(*expr.left, var);
                leftTerm->right = clone(*expr.right);

                auto rightTerm = std::make_unique<ExprNode>();
                rightTerm->kind = ExprNode::Kind::BINARY;
                rightTerm->binaryOp = "*";
                rightTerm->left = clone(*expr.left);
                rightTerm->right = diff(*expr.right, var);

                result->left = std::move(leftTerm);
                result->right = std::move(rightTerm);
            } else if (expr.binaryOp == "/") {
                // Quotient rule: (f/g)' = (f'g - fg') / g^2
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "/";
                
                auto numerator = std::make_unique<ExprNode>();
                numerator->kind = ExprNode::Kind::BINARY;
                numerator->binaryOp = "-";
                
                auto leftTerm = std::make_unique<ExprNode>();
                leftTerm->kind = ExprNode::Kind::BINARY;
                leftTerm->binaryOp = "*";
                leftTerm->left = diff(*expr.left, var);
                leftTerm->right = clone(*expr.right);

                auto rightTerm = std::make_unique<ExprNode>();
                rightTerm->kind = ExprNode::Kind::BINARY;
                rightTerm->binaryOp = "*";
                rightTerm->left = clone(*expr.left);
                rightTerm->right = diff(*expr.right, var);

                numerator->left = std::move(leftTerm);
                numerator->right = std::move(rightTerm);
                
                auto denominator = std::make_unique<ExprNode>();
                denominator->kind = ExprNode::Kind::BINARY;
                denominator->binaryOp = "^";
                denominator->left = clone(*expr.right);
                auto two = std::make_unique<ExprNode>();
                two->kind = ExprNode::Kind::NUMBER;
                two->number = 2.0;
                denominator->right = std::move(two);

                result->left = std::move(numerator);
                result->right = std::move(denominator);
            } else if (expr.binaryOp == "^") {
                // Power rule (base^const): (f^n)' = n * f^(n-1) * f'
                if (expr.right->kind == ExprNode::Kind::NUMBER) {
                    double n = expr.right->number;
                    
                    auto mul1 = std::make_unique<ExprNode>();
                    mul1->kind = ExprNode::Kind::BINARY;
                    mul1->binaryOp = "*";
                    
                    auto n_node = std::make_unique<ExprNode>();
                    n_node->kind = ExprNode::Kind::NUMBER;
                    n_node->number = n;
                    mul1->left = std::move(n_node);
                    
                    auto mul2 = std::make_unique<ExprNode>();
                    mul2->kind = ExprNode::Kind::BINARY;
                    mul2->binaryOp = "*";
                    
                    auto pow = std::make_unique<ExprNode>();
                    pow->kind = ExprNode::Kind::BINARY;
                    pow->binaryOp = "^";
                    pow->left = clone(*expr.left);
                    auto n_minus_1 = std::make_unique<ExprNode>();
                    n_minus_1->kind = ExprNode::Kind::NUMBER;
                    n_minus_1->number = n - 1.0;
                    pow->right = std::move(n_minus_1);
                    
                    mul2->left = std::move(pow);
                    mul2->right = diff(*expr.left, var);
                    
                    mul1->right = std::move(mul2);
                    result = std::move(mul1);
                }
            }
            break;
        }

        case ExprNode::Kind::CALL: {
            if (expr.callee == "sin") {
                // sin(f)' = cos(f) * f'
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "*";
                
                auto cos_call = std::make_unique<ExprNode>();
                cos_call->kind = ExprNode::Kind::CALL;
                cos_call->callee = "cos";
                cos_call->args.push_back(clone(*expr.args[0]));
                
                result->left = std::move(cos_call);
                result->right = diff(*expr.args[0], var);
            } else if (expr.callee == "cos") {
                // cos(f)' = -sin(f) * f'
                result->kind = ExprNode::Kind::BINARY;
                result->binaryOp = "*";
                
                auto neg_sin = std::make_unique<ExprNode>();
                neg_sin->kind = ExprNode::Kind::UNARY;
                neg_sin->unaryOp = "-";
                auto sin_call = std::make_unique<ExprNode>();
                sin_call->kind = ExprNode::Kind::CALL;
                sin_call->callee = "sin";
                sin_call->args.push_back(clone(*expr.args[0]));
                neg_sin->operand = std::move(sin_call);
                
                result->left = std::move(neg_sin);
                result->right = diff(*expr.args[0], var);
            }
            break;
        }
        default: break;
    }

    return simplify(std::move(result));
}

std::unique_ptr<ExprNode> Reconciler::simplify(std::unique_ptr<ExprNode> expr) {
    if (!expr) return nullptr;

    // Simple constant folding and identity simplification
    if (expr->kind == ExprNode::Kind::BINARY) {
        expr->left = simplify(std::move(expr->left));
        expr->right = simplify(std::move(expr->right));

        if (expr->left->kind == ExprNode::Kind::NUMBER && expr->right->kind == ExprNode::Kind::NUMBER) {
            double l = expr->left->number;
            double r = expr->right->number;
            auto n = std::make_unique<ExprNode>();
            n->kind = ExprNode::Kind::NUMBER;
            if (expr->binaryOp == "+") n->number = l + r;
            else if (expr->binaryOp == "-") n->number = l - r;
            else if (expr->binaryOp == "*") n->number = l * r;
            else if (expr->binaryOp == "/") n->number = l / r;
            else if (expr->binaryOp == "^") n->number = std::pow(l, r);
            return n;
        }

        if (expr->binaryOp == "*") {
            if (expr->left->kind == ExprNode::Kind::NUMBER && expr->left->number == 0.0) return clone(*expr->left);
            if (expr->right->kind == ExprNode::Kind::NUMBER && expr->right->number == 0.0) return clone(*expr->right);
            if (expr->left->kind == ExprNode::Kind::NUMBER && expr->left->number == 1.0) return std::move(expr->right);
            if (expr->right->kind == ExprNode::Kind::NUMBER && expr->right->number == 1.0) return std::move(expr->left);
        }
        if (expr->binaryOp == "+") {
            if (expr->left->kind == ExprNode::Kind::NUMBER && expr->left->number == 0.0) return std::move(expr->right);
            if (expr->right->kind == ExprNode::Kind::NUMBER && expr->right->number == 0.0) return std::move(expr->left);
        }
        if (expr->binaryOp == "-") {
            if (expr->right->kind == ExprNode::Kind::NUMBER && expr->right->number == 0.0) return std::move(expr->left);
        }
    } else if (expr->kind == ExprNode::Kind::UNARY) {
        expr->operand = simplify(std::move(expr->operand));
        if (expr->operand->kind == ExprNode::Kind::NUMBER) {
            auto n = std::make_unique<ExprNode>();
            n->kind = ExprNode::Kind::NUMBER;
            n->number = -expr->operand->number;
            return n;
        }
    }

    return expr;
}
