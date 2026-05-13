#pragma once
#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

// base for all AST nodes. every node tracks its line number
// so the evaluator can report where runtime errors happened
class ASTNode {
public:
    int line;
    ASTNode(int line) : line(line) {}
    virtual ~ASTNode() = default;
};

// expression types of AST nodes

class NumberLiteral : public ASTNode {
public:
    double value;
    NumberLiteral(int line, double value) : ASTNode(line), value(value) {}
};

class StringLiteral : public ASTNode {
public:
    std::string value;
    StringLiteral(int line, std::string value) : ASTNode(line), value(std::move(value)) {}
};

class BoolLiteral : public ASTNode {
public:
    bool value;
    BoolLiteral(int line, bool value) : ASTNode(line), value(value) {}
};

class NullLiteral : public ASTNode {
public:
    NullLiteral(int line) : ASTNode(line) {}
};

class Identifier : public ASTNode {
public:
    std::string name;
    Identifier(int line, std::string name) : ASTNode(line), name(std::move(name)) {}
};

class BinaryExpr : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    TokenType op;
    std::unique_ptr<ASTNode> right;
    BinaryExpr(int line, std::unique_ptr<ASTNode> left, TokenType op, std::unique_ptr<ASTNode> right)
        : ASTNode(line), left(std::move(left)), op(op), right(std::move(right)) {}
};

class UnaryExpr : public ASTNode {
public:
    TokenType op;
    std::unique_ptr<ASTNode> operand;
    UnaryExpr(int line, TokenType op, std::unique_ptr<ASTNode> operand)
        : ASTNode(line), op(op), operand(std::move(operand)) {}
};

// callee is an expression, not just a name. this lets chained calls
// like makeCounter()() work, bc the first call returns a function
// that the second call invokes
class CallExpr : public ASTNode {
public:
    std::unique_ptr<ASTNode> callee;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    CallExpr(int line, std::unique_ptr<ASTNode> callee, std::vector<std::unique_ptr<ASTNode>> arguments)
        : ASTNode(line), callee(std::move(callee)), arguments(std::move(arguments)) {}
};

// statement types of AST nodes

class LetStatement : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> initializer;
    LetStatement(int line, std::string name, std::unique_ptr<ASTNode> initializer)
        : ASTNode(line), name(std::move(name)), initializer(std::move(initializer)) {}
};

// assignment is a statement, not an expression (simpler than lox)
class Assignment : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> value;
    Assignment(int line, std::string name, std::unique_ptr<ASTNode> value)
        : ASTNode(line), name(std::move(name)), value(std::move(value)) {}
};

// elseBranch can be a Block (else), another IfStatement (else if), or nullptr
class IfStatement : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
    IfStatement(int line, std::unique_ptr<ASTNode> condition,
                std::unique_ptr<ASTNode> thenBranch, std::unique_ptr<ASTNode> elseBranch)
        : ASTNode(line), condition(std::move(condition)),
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
};

class WhileStatement : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    WhileStatement(int line, std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : ASTNode(line), condition(std::move(condition)), body(std::move(body)) {}
};

class FunctionDecl : public ASTNode {
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
    FunctionDecl(int line, std::string name, std::vector<std::string> params, std::unique_ptr<ASTNode> body)
        : ASTNode(line), name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};

class ReturnStatement : public ASTNode {
public:
    std::unique_ptr<ASTNode> value; // nullptr for bare "return;"
    ReturnStatement(int line, std::unique_ptr<ASTNode> value)
        : ASTNode(line), value(std::move(value)) {}
};

// print is a keyword, not a function
class PrintStatement : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    PrintStatement(int line, std::unique_ptr<ASTNode> expression)
        : ASTNode(line), expression(std::move(expression)) {}
};

class Block : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    Block(int line, std::vector<std::unique_ptr<ASTNode>> statements)
        : ASTNode(line), statements(std::move(statements)) {}
};

class ExpressionStatement : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    ExpressionStatement(int line, std::unique_ptr<ASTNode> expression)
        : ASTNode(line), expression(std::move(expression)) {}
};
