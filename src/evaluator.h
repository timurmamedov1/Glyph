#pragma once
#include "parser.h"
#include "error.h"
#include <variant>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <cmath>
#include <sstream>

// forward declare so Function can hold shared_ptr<Environment>
class Environment;

// Function needs to be fully defined before Value bc variant
// needs complete types. shared_ptr<Environment> works with
// just the forward declaration
struct Function {
    std::string name;
    std::vector<std::string> params;
    ASTNode* body; // non-owning, AST must outlive execution
    std::shared_ptr<Environment> closure;
};

// the runtime value type. holds any glyph value
using Value = std::variant<double, bool, std::string, std::nullptr_t, Function>;

// converts a value to its string representation for print and error messages.
// integers display without decimal points (10 not 10.000000)
std::string valueToString(const Value& v) {
    if (auto* n = std::get_if<double>(&v)) {
        if (*n == std::floor(*n) && std::isfinite(*n)) {
            return std::to_string((long long)*n);
        }
        // ostringstream avoids the trailing zeros that std::to_string adds
        std::ostringstream oss;
        oss << *n;
        return oss.str();
    }
    if (auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (std::holds_alternative<std::nullptr_t>(v)) return "null";
    if (auto* f = std::get_if<Function>(&v)) return "<fn " + f->name + ">";
    return "unknown";
}

// glyph truthiness: false, null, 0, and "" are falsy. everything else is truthy
bool isTruthy(const Value& v) {
    if (auto* b = std::get_if<bool>(&v)) return *b;
    if (std::holds_alternative<std::nullptr_t>(v)) return false;
    if (auto* n = std::get_if<double>(&v)) return *n != 0.0;
    if (auto* s = std::get_if<std::string>(&v)) return !s->empty();
    return true;
}

std::string valueTypeName(const Value& v) {
    if (std::holds_alternative<double>(v)) return "number";
    if (std::holds_alternative<bool>(v)) return "bool";
    if (std::holds_alternative<std::string>(v)) return "string";
    if (std::holds_alternative<std::nullptr_t>(v)) return "null";
    if (std::holds_alternative<Function>(v)) return "function";
    return "unknown";
}

// manual equality check bc variant == requires all types to support it
// and Function doesnt. different types are never equal, functions are never equal
Value valuesEqual(const Value& a, const Value& b) {
    if (a.index() != b.index()) return false;
    if (auto* l = std::get_if<double>(&a)) return *l == *std::get_if<double>(&b);
    if (auto* l = std::get_if<bool>(&a)) return *l == *std::get_if<bool>(&b);
    if (auto* l = std::get_if<std::string>(&a)) return *l == *std::get_if<std::string>(&b);
    if (std::holds_alternative<std::nullptr_t>(a)) return true; // null == null
    return false; // functions are never equal
}

// linked scope chain. each environment has a pointer to its parent,
// so variable lookups walk up the chain until they find the name.
// shared_ptr bc closures need to keep their captured scope alive
class Environment {
public:
    std::shared_ptr<Environment> parent;
    std::unordered_map<std::string, Value> variables;

    Environment() : parent(nullptr) {}
    Environment(std::shared_ptr<Environment> parent) : parent(std::move(parent)) {}

    void define(const std::string& name, const Value& value) {
        variables[name] = value;
    }

    // walks the parent chain looking for where the variable was defined
    Value get(const std::string& name, int line) {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        if (parent) return parent->get(name, line);
        throw GlyphError(line, "Undefined variable '" + name + "'.");
    }

    // walks the chain and updates the variable where it was originally defined
    void set(const std::string& name, const Value& value, int line) {
        auto it = variables.find(name);
        if (it != variables.end()) { it->second = value; return; }
        if (parent) { parent->set(name, value, line); return; }
        throw GlyphError(line, "Undefined variable '" + name + "'.");
    }
};

// thrown by return statements, caught by call expressions.
// not an error, just a control flow mechanism to unwind the stack (same as jlox)
class ReturnValue : public std::exception {
public:
    Value value;
    ReturnValue(Value v) : value(std::move(v)) {}
};

class Evaluator {
public:
    std::shared_ptr<Environment> environment;

    Evaluator() : environment(std::make_shared<Environment>()) {}

    // evaluates an AST node and returns its value
    Value evaluate(ASTNode* node) {
        // literals
        if (auto* n = dynamic_cast<NumberLiteral*>(node))
            return n->value;
        if (auto* s = dynamic_cast<StringLiteral*>(node))
            return s->value;
        if (auto* b = dynamic_cast<BoolLiteral*>(node))
            return b->value;
        if (dynamic_cast<NullLiteral*>(node))
            return nullptr;

        // binary operators
        if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
            // and/or short circuit, so we evaluate the right side only if needed.
            // they return the determining value, not a bool (python/js semantics)
            if (bin->op == TokenType::AND) {
                Value left = evaluate(bin->left.get());
                if (!isTruthy(left)) return left;
                return evaluate(bin->right.get());
            }
            if (bin->op == TokenType::OR) {
                Value left = evaluate(bin->left.get());
                if (isTruthy(left)) return left;
                return evaluate(bin->right.get());
            }

            Value left = evaluate(bin->left.get());
            Value right = evaluate(bin->right.get());

            switch (bin->op) {
                // arithmetic (both must be numbers)
                case TokenType::MINUS:
                case TokenType::STAR:
                case TokenType::SLASH:
                case TokenType::PERCENT: {
                    auto* l = std::get_if<double>(&left);
                    auto* r = std::get_if<double>(&right);
                    if (!l || !r)
                        throw GlyphError(bin->line, "Operands must be numbers.");
                    if (bin->op == TokenType::MINUS) return *l - *r;
                    if (bin->op == TokenType::STAR) return *l * *r;
                    if (bin->op == TokenType::SLASH) {
                        if (*r == 0) throw GlyphError(bin->line, "Division by zero.");
                        return *l / *r;
                    }
                    // percent
                    if (*r == 0) throw GlyphError(bin->line, "Modulo by zero.");
                    return std::fmod(*l, *r);
                }

                // plus is overloaded: numbers add, strings concatenate
                case TokenType::PLUS: {
                    auto* ln = std::get_if<double>(&left);
                    auto* rn = std::get_if<double>(&right);
                    if (ln && rn) return *ln + *rn;
                    auto* ls = std::get_if<std::string>(&left);
                    auto* rs = std::get_if<std::string>(&right);
                    if (ls && rs) return *ls + *rs;
                    throw GlyphError(bin->line, "Operands must be two numbers or two strings.");
                }

                // comparison (both must be numbers)
                case TokenType::LESS:
                case TokenType::LESS_EQUAL:
                case TokenType::GREATER:
                case TokenType::GREATER_EQUAL: {
                    auto* l = std::get_if<double>(&left);
                    auto* r = std::get_if<double>(&right);
                    if (!l || !r)
                        throw GlyphError(bin->line, "Operands must be numbers.");
                    if (bin->op == TokenType::LESS) return *l < *r;
                    if (bin->op == TokenType::LESS_EQUAL) return *l <= *r;
                    if (bin->op == TokenType::GREATER) return *l > *r;
                    return *l >= *r;
                }

                // equality (any types, different types are never equal).
                // cant use variant == bc Function has no operator==
                case TokenType::EQUAL_EQUAL: return valuesEqual(left, right);
                case TokenType::BANG_EQUAL: return !std::get<bool>(valuesEqual(left, right));

                default:
                    throw GlyphError(bin->line, "Unknown binary operator.");
            }
        }

        // unary operators
        if (auto* unary = dynamic_cast<UnaryExpr*>(node)) {
            Value operand = evaluate(unary->operand.get());
            if (unary->op == TokenType::MINUS) {
                auto* n = std::get_if<double>(&operand);
                if (!n) throw GlyphError(unary->line, "Operand must be a number.");
                return -(*n);
            }
            if (unary->op == TokenType::BANG) {
                return !isTruthy(operand);
            }
            throw GlyphError(unary->line, "Unknown unary operator.");
        }

        // variable lookup
        if (auto* id = dynamic_cast<Identifier*>(node))
            return environment->get(id->name, id->line);

        // let declaration
        if (auto* let = dynamic_cast<LetStatement*>(node)) {
            Value val = evaluate(let->initializer.get());
            environment->define(let->name, val);
            return nullptr;
        }

        // assignment (walks the scope chain to find where it was defined)
        if (auto* assign = dynamic_cast<Assignment*>(node)) {
            Value val = evaluate(assign->value.get());
            environment->set(assign->name, val, assign->line);
            return nullptr;
        }

        // if/else. elseBranch can be a Block, another IfStatement (else if), or nullptr
        if (auto* ifStmt = dynamic_cast<IfStatement*>(node)) {
            Value cond = evaluate(ifStmt->condition.get());
            if (isTruthy(cond)) {
                evaluate(ifStmt->thenBranch.get());
            } else if (ifStmt->elseBranch) {
                evaluate(ifStmt->elseBranch.get());
            }
            return nullptr;
        }

        // while loop
        if (auto* whileStmt = dynamic_cast<WhileStatement*>(node)) {
            while (isTruthy(evaluate(whileStmt->condition.get()))) {
                evaluate(whileStmt->body.get());
            }
            return nullptr;
        }

        // function declaration. captures the current environment for closures
        if (auto* fnDecl = dynamic_cast<FunctionDecl*>(node)) {
            Function fn{fnDecl->name, fnDecl->params, fnDecl->body.get(), environment};
            environment->define(fnDecl->name, fn);
            return nullptr;
        }

        // function call. evaluates the callee (which can be any expression),
        // checks its a function, binds args to params in a new scope
        // that chains off the closure, then executes the body
        if (auto* call = dynamic_cast<CallExpr*>(node)) {
            Value callee = evaluate(call->callee.get());
            auto* fn = std::get_if<Function>(&callee);
            if (!fn)
                throw GlyphError(call->line, "Can only call functions.");
            if (call->arguments.size() != fn->params.size())
                throw GlyphError(call->line, "Expected " + std::to_string(fn->params.size())
                    + " arguments but got " + std::to_string(call->arguments.size()) + ".");

            // new environment with closure as parent, not the current scope
            auto callEnv = std::make_shared<Environment>(fn->closure);
            for (int i = 0; i < (int)fn->params.size(); i++) {
                callEnv->define(fn->params[i], evaluate(call->arguments[i].get()));
            }

            auto previous = environment;
            environment = callEnv;
            Value result = nullptr;
            try {
                evaluate(fn->body);
            } catch (ReturnValue& ret) {
                result = std::move(ret.value);
            }
            environment = previous;
            return result;
        }

        // return throws a ReturnValue exception to unwind back to the call site
        if (auto* ret = dynamic_cast<ReturnStatement*>(node)) {
            Value val = nullptr;
            if (ret->value) val = evaluate(ret->value.get());
            throw ReturnValue(std::move(val));
        }

        // print
        if (auto* p = dynamic_cast<PrintStatement*>(node)) {
            Value val = evaluate(p->expression.get());
            std::cout << valueToString(val) << std::endl;
            return nullptr;
        }

        // blocks create a new scope
        if (auto* block = dynamic_cast<Block*>(node)) {
            auto blockEnv = std::make_shared<Environment>(environment);
            auto previous = environment;
            environment = blockEnv;
            for (auto& stmt : block->statements) {
                evaluate(stmt.get());
            }
            environment = previous;
            return nullptr;
        }

        // expression statements just evaluate and discard
        if (auto* expr = dynamic_cast<ExpressionStatement*>(node)) {
            evaluate(expr->expression.get());
            return nullptr;
        }

        throw GlyphError(node->line, "Unknown AST node type.");
    }
};
