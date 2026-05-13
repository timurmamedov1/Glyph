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

// recursive descent parser. each precedence level is its own method,
// calling down to the next tighter level. statements dispatch by
// looking at the current token type
class Parser {
public:
    std::vector<Token> tokens;
    int current;
    int functionDepth; // tracks nesting so we can reject return outside functions

    Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0), functionDepth(0) {}

    // entry point. parses tokens into a list of statements
    std::vector<std::unique_ptr<ASTNode>> parse() {
        std::vector<std::unique_ptr<ASTNode>> statements;
        while (!isAtEnd()) {
            statements.push_back(parseStatement());
        }
        return statements;
    }

private:
    bool isAtEnd() { return peek().type == TokenType::EOF_TOKEN; }
    Token peek() { return tokens[current]; }
    Token peekNext() { return tokens[current + 1]; } // only used for assignment lookahead
    Token advance() { return tokens[current++]; }

    bool match(TokenType type) {
        if (peek().type != type) return false;
        advance();
        return true;
    }

    Token expect(TokenType type, const std::string& msg) {
        if (peek().type == type) return advance();
        throw GlyphError(peek().line, msg);
    }

    // statement parsing functions based on the first token. if its not a statement,
    // we fall back to parsing an expression statement

    std::unique_ptr<ASTNode> parseStatement() {
        switch (peek().type) {
            case TokenType::LET:    return parseLetStatement();
            case TokenType::IF:     return parseIfStatement();
            case TokenType::WHILE:  return parseWhileStatement();
            case TokenType::FN:     return parseFunctionDecl();
            case TokenType::RETURN: return parseReturnStatement();
            case TokenType::PRINT:  return parsePrintStatement();
            case TokenType::LBRACE: return parseBlock();
            default:                return parseAssignmentOrExprStmt();
        }
    }

    std::unique_ptr<ASTNode> parseLetStatement() {
        int line = advance().line;
        Token name = expect(TokenType::IDENTIFIER, "Expected variable name after 'let'.");
        expect(TokenType::EQUAL, "Expected '=' after variable name.");
        auto initializer = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
        return std::make_unique<LetStatement>(line, name.value, std::move(initializer));
    }

    // else if is handled by recursing into parseIfStatement for the else branch
    std::unique_ptr<ASTNode> parseIfStatement() {
        int line = advance().line;
        expect(TokenType::LPAREN, "Expected '(' after 'if'.");
        auto condition = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after if condition.");
        auto thenBranch = parseBlock();

        std::unique_ptr<ASTNode> elseBranch = nullptr;
        if (match(TokenType::ELSE)) {
            if (peek().type == TokenType::IF) {
                elseBranch = parseIfStatement();
            } else {
                elseBranch = parseBlock();
            }
        }
        return std::make_unique<IfStatement>(line, std::move(condition),
                   std::move(thenBranch), std::move(elseBranch));
    }

    std::unique_ptr<ASTNode> parseWhileStatement() {
        int line = advance().line;
        expect(TokenType::LPAREN, "Expected '(' after 'while'.");
        auto condition = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after while condition.");
        auto body = parseBlock();
        return std::make_unique<WhileStatement>(line, std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> parseFunctionDecl() {
        int line = advance().line;
        Token name = expect(TokenType::IDENTIFIER, "Expected function name after 'fn'.");
        expect(TokenType::LPAREN, "Expected '(' after function name.");

        std::vector<std::string> params;
        if (peek().type != TokenType::RPAREN) {
            do {
                Token param = expect(TokenType::IDENTIFIER, "Expected parameter name.");
                params.push_back(param.value);
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')' after parameters.");

        functionDepth++;
        auto body = parseBlock();
        functionDepth--;

        return std::make_unique<FunctionDecl>(line, name.value, std::move(params), std::move(body));
    }

    std::unique_ptr<ASTNode> parseReturnStatement() {
        int line = advance().line;
        if (functionDepth == 0) {
            throw GlyphError(line, "Cannot use 'return' outside of a function.");
        }

        std::unique_ptr<ASTNode> value = nullptr;
        if (peek().type != TokenType::SEMICOLON) {
            value = parseExpression();
        }
        expect(TokenType::SEMICOLON, "Expected ';' after return value.");
        return std::make_unique<ReturnStatement>(line, std::move(value));
    }

    std::unique_ptr<ASTNode> parsePrintStatement() {
        int line = advance().line;
        expect(TokenType::LPAREN, "Expected '(' after 'print'.");
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after print expression.");
        expect(TokenType::SEMICOLON, "Expected ';' after print statement.");
        return std::make_unique<PrintStatement>(line, std::move(expr));
    }

    std::unique_ptr<ASTNode> parseBlock() {
        int line = expect(TokenType::LBRACE, "Expected '{'.").line;
        std::vector<std::unique_ptr<ASTNode>> statements;
        // isAtEnd check prevents infinite loop if closing brace is missing
        while (peek().type != TokenType::RBRACE && !isAtEnd()) {
            statements.push_back(parseStatement());
        }
        expect(TokenType::RBRACE, "Expected '}'.");
        return std::make_unique<Block>(line, std::move(statements));
    }

    // lookahead: if current is IDENTIFIER and next is '=', its assignment.
    // otherwise parse as expression statement
    std::unique_ptr<ASTNode> parseAssignmentOrExprStmt() {
        int line = peek().line;

        if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::EQUAL) {
            std::string name = advance().value;
            advance(); // consume '='
            auto value = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after assignment.");
            return std::make_unique<Assignment>(line, std::move(name), std::move(value));
        }

        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after expression.");
        return std::make_unique<ExpressionStatement>(line, std::move(expr));
    }

    // expression parsing (precedence climbing). each method handles one
    // level: or > and > equality > comparison > addition > multiplication
    // > unary > call > primary

    std::unique_ptr<ASTNode> parseExpression() { return parseOr(); }

    std::unique_ptr<ASTNode> parseOr() {
        auto left = parseAnd();
        while (peek().type == TokenType::OR) {
            int line = advance().line;
            auto right = parseAnd();
            left = std::make_unique<BinaryExpr>(line, std::move(left), TokenType::OR, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAnd() {
        auto left = parseEquality();
        while (peek().type == TokenType::AND) {
            int line = advance().line;
            auto right = parseEquality();
            left = std::make_unique<BinaryExpr>(line, std::move(left), TokenType::AND, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseEquality() {
        auto left = parseComparison();
        while (peek().type == TokenType::EQUAL_EQUAL || peek().type == TokenType::BANG_EQUAL) {
            Token op = advance();
            auto right = parseComparison();
            left = std::make_unique<BinaryExpr>(op.line, std::move(left), op.type, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseComparison() {
        auto left = parseAddition();
        while (peek().type == TokenType::LESS || peek().type == TokenType::LESS_EQUAL ||
               peek().type == TokenType::GREATER || peek().type == TokenType::GREATER_EQUAL) {
            Token op = advance();
            auto right = parseAddition();
            left = std::make_unique<BinaryExpr>(op.line, std::move(left), op.type, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAddition() {
        auto left = parseMultiplication();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            Token op = advance();
            auto right = parseMultiplication();
            left = std::make_unique<BinaryExpr>(op.line, std::move(left), op.type, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseMultiplication() {
        auto left = parseUnary();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH ||
               peek().type == TokenType::PERCENT) {
            Token op = advance();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpr>(op.line, std::move(left), op.type, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseUnary() {
        if (peek().type == TokenType::MINUS || peek().type == TokenType::BANG) {
            Token op = advance();
            auto operand = parseUnary();
            return std::make_unique<UnaryExpr>(op.line, op.type, std::move(operand));
        }
        return parseCall();
    }

    // handles chained calls like makeCounter()()
    std::unique_ptr<ASTNode> parseCall() {
        auto expr = parsePrimary();
        while (peek().type == TokenType::LPAREN) {
            int line = advance().line;
            std::vector<std::unique_ptr<ASTNode>> args;
            if (peek().type != TokenType::RPAREN) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments.");
            expr = std::make_unique<CallExpr>(line, std::move(expr), std::move(args));
        }
        return expr;
    }

    std::unique_ptr<ASTNode> parsePrimary() {
        Token token = peek();

        switch (token.type) {
            case TokenType::NUMBER:
                advance();
                return std::make_unique<NumberLiteral>(token.line, std::stod(token.value));
            case TokenType::STRING:
                advance();
                return std::make_unique<StringLiteral>(token.line, token.value);
            case TokenType::TRUE:
                advance();
                return std::make_unique<BoolLiteral>(token.line, true);
            case TokenType::FALSE:
                advance();
                return std::make_unique<BoolLiteral>(token.line, false);
            case TokenType::NULL_LITERAL:
                advance();
                return std::make_unique<NullLiteral>(token.line);
            case TokenType::IDENTIFIER:
                advance();
                return std::make_unique<Identifier>(token.line, token.value);
            case TokenType::LPAREN: {
                advance();
                auto expr = parseExpression();
                expect(TokenType::RPAREN, "Expected ')' after expression.");
                return expr;
            }
            default:
                throw GlyphError(token.line, "Unexpected token '" + token.value + "'.");
        }
    }
};
