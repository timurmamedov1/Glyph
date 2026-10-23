# Glyph

[![CI](https://github.com/timurmamedov1/Glyph/actions/workflows/ci.yml/badge.svg)](https://github.com/timurmamedov1/Glyph/actions/workflows/ci.yml)

A dynamically-typed programming language and interpreter built from scratch in C++17 using no external libraries.

Built using [Crafting Interpreters](https://craftinginterpreters.com/) Part II (jlox, chapters 4–10), plus a mark-and-sweep garbage collector modeled on clox's (Part III, chapter 26).

## The Language

```
fn fib(n) {
    if (n <= 1) { 
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

for (let i = 0; i < 10; i = i + 1) {
    print(fib(i));
}
```

Glyph supports numbers, strings, booleans, null, variables, control flow, first-class functions, closures, and recursion.

## Features

- **Three stages:** source => lexer => parser => tree-walk evaluator
- **First-class functions** with lexical closures
- **Garbage collected:** a mark-and-sweep collector frees scopes once nothing can reach them, including the cycles closures create
- **Recursive descent parser** with precedence climbing
- **REPL** with multi-line input (brace balancing)
- **Truthiness:** `false`, `null`, `0`, and `""` are falsy
- **Short-circuit evaluation:** `and`/`or` return the determining value

## Build & Run

```bash
make              # compile
make test         # run the test suite
make gctest       # garbage collector unit tests
make sanitize     # test suite under ASan + UBSan with the GC running on every allocation
./glyph           # REPL mode
./glyph file.gl   # file mode
```

Requires `g++` with C++17 support.

## Syntax Overview

```
// variables (initialization required)
let name = "Glyph";
let x = 42;
x = 100;

// control flow
if (x > 50) {
    print("big");
} else if (x > 25) {
    print("medium");
} else {
    print("small");
}

// loops
while (x > 0) {
    x = x - 10;
}

for (let i = 0; i < 5; i = i + 1) {
    print(i);
}

// functions and closures
fn makeCounter() {
    let count = 0;
    fn increment() {
        count = count + 1;
        return count;
    }
    return increment;
}
let c = makeCounter();
print(c());  // 1
print(c());  // 2
```

### Differences from Lox

- `let` instead of `var`, `fn` instead of `fun`, `null` instead of `nil`
- `%` modulo operator
- `0` and `""` are falsy
- `else if` supported directly
- No classes, no inheritance
- Assignment is a statement, not an expression

## Memory Management

Scopes are owned by a `Heap` and freed by a mark-and-sweep garbage collector (`src/evaluator.h`). Reference counting can't free them: a function stored in the scope it closes over points back at that scope, so the original `shared_ptr` version leaked every closure. Now memory stays flat no matter how many closures a program creates and throws away.

- **Roots:** the current scope, the scopes that enclosing blocks and calls will return to, and scopes held only by C++ locals mid-evaluation (a call's new scope while its arguments run, the left operand of a binary operator)
- **Mark:** an explicit gray stack instead of recursion, so a 100k-long chain of closures can't overflow the C++ stack
- **Sweep:** frees everything unmarked, then schedules the next collection for when the live count doubles (minimum 1024)
- **Testing:** `make sanitize` builds with `GC_STRESS`, which collects on every allocation, under ASan and UBSan, so a missing root shows up right away as a use-after-free. `make gctest` checks the heap directly: garbage gets freed while a program runs, and live closures survive.

## Tech Stack

- **Language:** C++17
- **Build:** Make
- **Architecture:** header-only (singular .cpp file since project is small enough)
- **Memory:** mark-and-sweep GC for scopes, `unique_ptr` for the AST
- **External dependencies:** none
