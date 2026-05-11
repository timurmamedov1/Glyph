#pragma once
#include <stdexcept>
#include <string>

// single error class for all phases (lex, parse, runtime).
// stores the line number so error messages can show [line N] Error: ...
class GlyphError : public std::runtime_error {
public:
    int line;
    GlyphError(int line, const std::string& msg) : std::runtime_error(msg), line(line) {}
};
