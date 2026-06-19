# Glyph

A dynamically-typed programming language and interpreter built from scratch in C++17 using no external libraries.

Built using [Crafting Interpreters](https://craftinginterpreters.com/) Part II (jlox, chapters 4–10).

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
- **Recursive descent parser** with precedence climbing
- **REPL** with multi-line input (brace balancing)
- **Truthiness:** `false`, `null`, `0`, and `""` are falsy
- **Short-circuit evaluation:** `and`/`or` return the determining value

## Build & Run

```bash
make              # compile
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

## Tech Stack

- **Language:** C++17
- **Build:** Make
- **Architecture:** header-only (singular .cpp file since project is small enough)
- **External dependencies:** none
