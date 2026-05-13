#include "parser.h"
#include "lexer.h"
#include "error.h"
#include <iostream>
#include <fstream>
#include <sstream>

// format and print a glyph error
void printError(const GlyphError& e) {
    std::cerr << "[line " << e.line << "] Error: " << e.what() << std::endl;
}

// reads a file, tokenizes, and parses it.
// no evaluator yet so it just confirms parsing succeeds
void runFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    }

    std::stringstream ss;
    ss << file.rdbuf();

    try {
        Lexer lexer(ss.str());
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
    } catch (const GlyphError& e) {
        printError(e);
        exit(1);
    }
}

// interactive mode. reads one line at a time, tokenizes and parses.
// no evaluator yet so theres no output, just error checking
void runRepl() {
    std::cout << "Glyph" << std::endl;
    std::string line;

    while (1) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        try {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto ast = parser.parse();
        } catch (const GlyphError& e) {
            printError(e);
        }
    }
}

// we have two modes: "glyph" ran with no args starts REPL mode,
// "glyph file.gl" (singular file) runs that file
// no multiple file support (yet) due to no imports or modules (yet)
int main(int argc, char* argv[]) {
    if (argc == 1) {
        runRepl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        std::cerr << "Usage: glyph [file.gl]" << std::endl;
        return 1;
    }
    return 0;
}