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
