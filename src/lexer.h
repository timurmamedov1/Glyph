#pragma once
#include "error.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>

enum class TokenType {
    // literals
    NUMBER, STRING, IDENTIFIER,

    // keywords
    LET, IF, ELSE, WHILE, FOR, FN, RETURN,
    TRUE, FALSE, NULL_LITERAL,
    AND, OR,
    PRINT,

    // operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,

    // delimiters
    LPAREN, RPAREN, LBRACE, RBRACE,
    COMMA, SEMICOLON,

    // EOF is a macro in cstdio so we cant just use that
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value; // the raw lexeme
    int line;          // 1-indexed
};

class Lexer {
public:
    std::string source;
    int current;
    int line;
    std::vector<Token> tokens;

    // keyword lookup, populated once per instance
    std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::LET},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"fn", TokenType::FN},
        {"return", TokenType::RETURN},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
        {"null", TokenType::NULL_LITERAL},
        {"and", TokenType::AND},
        {"or", TokenType::OR},
        {"print", TokenType::PRINT},
    };

    // takes in raw source code, call tokenize() to get the token list
    Lexer(const std::string& src) : source(src), current(0), line(1) {}

    // walks through the source and returns all tokens, with an EOF_TOKEN at the end
    std::vector<Token> tokenize() {
        while (!isAtEnd()) {
            scanToken();
        }
        tokens.push_back({TokenType::EOF_TOKEN, "", line});
        return tokens;
    }

private:
    // below functions are helpers for moving through the source

    bool isAtEnd() { return current >= (int)source.size(); }

    // look at current char without consuming it
    char peek() {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    // look one char ahead (used for fractional numbers, two-char tokens)
    char peekNext() {
        if (current + 1 >= (int)source.size()) return '\0';
        return source[current + 1];
    }

    // consume current char and return it
    char advance() { return source[current++]; }

    // consume current char only if it matches. used for two-char tokens
    // like == where we need to check if the second char is there
    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }

    void addToken(TokenType type, const std::string& value) {
        tokens.push_back({type, value, line});
    }

    // reads the next token from source. handles single char tokens,
    // two char tokens (==, !=, etc), comments, strings, numbers, identifiers
    void scanToken() {
        char c = advance();

        switch (c) {
            case '(':
                addToken(TokenType::LPAREN, "("); break;
            case ')':
                addToken(TokenType::RPAREN, ")"); break;
            case '{':
                addToken(TokenType::LBRACE, "{"); break;
            case '}':
                addToken(TokenType::RBRACE, "}"); break;
            case ',':
                addToken(TokenType::COMMA, ","); break;
            case ';':
                addToken(TokenType::SEMICOLON, ";"); break;
            case '+':
                addToken(TokenType::PLUS, "+"); break;
            case '-':
                addToken(TokenType::MINUS, "-"); break;
            case '*':
                addToken(TokenType::STAR, "*"); break;
            case '%':
                addToken(TokenType::PERCENT, "%"); break;

            // two char tokens. check for the second char, fall back to single
            case '=':
                if (match('=')) addToken(TokenType::EQUAL_EQUAL, "==");
                else addToken(TokenType::EQUAL, "=");
                break;
            case '!':
                if (match('=')) addToken(TokenType::BANG_EQUAL, "!=");
                else addToken(TokenType::BANG, "!");
                break;
            case '<':
                if (match('=')) addToken(TokenType::LESS_EQUAL, "<=");
                else addToken(TokenType::LESS, "<");
                break;
            case '>':
                if (match('=')) addToken(TokenType::GREATER_EQUAL, ">=");
                else addToken(TokenType::GREATER, ">");
                break;

            // slash is either division or start of a comment
            case '/':
                if (match('/')) {
                    while (!isAtEnd() && peek() != '\n') advance();
                } else {
                    addToken(TokenType::SLASH, "/");
                }
                break;

            // whitespace
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                line++; break;

            case '"':
                scanString(); break;

            default:
                if (std::isdigit(c)) {
                    current--; // back up so scanNumber gets the full number
                    scanNumber();
                } else if (std::isalpha(c) || c == '_') {
                    current--;
                    scanIdentifier();
                } else {
                    throw GlyphError(line, std::string("Unexpected character '") + c + "'.");
                }
                break;
        }
    }

    // scans an integer or decimal number (e.g. 42, 3.14)
    void scanNumber() {
        int start = current;
        while (!isAtEnd() && std::isdigit(peek())) advance();

        // fractional part. only consume the dot if theres digits after it,
        // otherwise something like "123." would eat the dot and leave no digits
        if (!isAtEnd() && peek() == '.' && std::isdigit(peekNext())) {
            advance(); // consume '.'
            while (!isAtEnd() && std::isdigit(peek())) advance();
        }

        addToken(TokenType::NUMBER, source.substr(start, current - start));
    }

    // scans a string literal, handles escape sequences (\n, \t, \\, \").
    // the value stored in the token is already unescaped
    void scanString() {
        // opening quote already consumed by scanToken
        std::string value;
        int startLine = line;

        while (!isAtEnd() && peek() != '"') {
            if (peek() == '\n') line++;

            if (peek() == '\\') {
                advance();
                if (isAtEnd()) throw GlyphError(startLine, "Unterminated string.");
                char escaped = advance();
                switch (escaped) {
                    case 'n':
                        value += '\n'; break;
                    case 't':
                        value += '\t'; break;
                    case '\\':
                        value += '\\'; break;
                    case '"':
                        value += '"';  break;
                    default:
                        // unrecognized escape, just keep both chars
                        value += '\\';
                        value += escaped;
                        break;
                }
            } else {
                value += advance();
            }
        }

        if (isAtEnd()) throw GlyphError(startLine, "Unterminated string.");
        advance(); // closing quote

        addToken(TokenType::STRING, value);
    }

    // scans an identifier or keyword. identifiers start with a letter or _,
    // then we check the keywords map to see if its actually a reserved word
    void scanIdentifier() {
        int start = current;
        while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) advance();

        std::string text = source.substr(start, current - start);

        // check if its a keyword, otherwise its a regular identifier
        auto it = keywords.find(text);
        if (it != keywords.end()) {
            addToken(it->second, text);
        } else {
            addToken(TokenType::IDENTIFIER, text);
        }
    }
};