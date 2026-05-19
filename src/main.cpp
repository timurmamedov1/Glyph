#include "evaluator.h"
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

// reads a file and runs it through the full pipeline
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

        Evaluator evaluator;
        for (auto& node : ast) {
            evaluator.evaluate(node.get());
        }
    } catch (const GlyphError& e) {
        printError(e);
        exit(1);
    }
}

// interactive mode. environment persists across lines so
// variables defined on one line are available on the next.
// supports multi-line input by tracking brace depth
void runRepl() {
    std::cout << "Glyph" << std::endl;
    std::string line;
    Evaluator evaluator;
    // accumulate ASTs so function bodies (raw pointers) stay alive
    std::vector<std::vector<std::unique_ptr<ASTNode>>> allAsts;

    while (1) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        // count braces to detect multi-line input. if theres more
        // opens than closes, keep reading with the continuation prompt
        std::string input = line;
        int depth = 0;
        for (char c : input) {
            if (c == '{') depth++;
            if (c == '}') depth--;
        }
        while (depth > 0) {
            std::cout << "... " << std::flush;
            if (!std::getline(std::cin, line)) break;
            input += "\n" + line;
            for (char c : line) {
                if (c == '{') depth++;
                if (c == '}') depth--;
            }
        }

        try {
            Lexer lexer(input);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto ast = parser.parse();

            for (auto& node : ast) {
                evaluator.evaluate(node.get());
            }
            // keep the AST alive so function body pointers dont dangle
            allAsts.push_back(std::move(ast));
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