// gc unit tests. the .gl programs check that output stays right, these check
// the heap itself: garbage really gets freed while a program runs, live
// closures survive, and errors dont leave dead scopes rooted. missing roots
// get caught by make sanitize instead (GC_STRESS + asan), not here
#include "../src/evaluator.h"
#include <iostream>

int failures = 0;

// like assert but keeps going so one run reports every failure
#define CHECK(cond) do { \
    if (!(cond)) { \
        std::cout << "FAIL " << __func__ << " line " << __LINE__ << ": " #cond << std::endl; \
        failures++; \
    } \
} while (0)

// asts have to outlive the evaluators bc functions hold raw pointers into them
std::vector<std::vector<std::unique_ptr<ASTNode>>> asts;

void run(Evaluator& ev, const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    asts.push_back(parser.parse());
    for (auto& node : asts.back()) {
        ev.evaluate(node.get());
    }
}

size_t liveEnvs(Evaluator& ev) {
    return ev.heap.objects.size();
}

// true if the heap still owns env, ie it hasnt been freed
bool alive(Evaluator& ev, Environment* env) {
    for (auto& obj : ev.heap.objects) {
        if (obj.get() == env) return true;
    }
    return false;
}

const std::string makeCounter = R"(
fn makeCounter() {
    let count = 0;
    fn increment() {
        count = count + 1;
        return count;
    }
    return increment;
}
)";

// every makeCounter() call leaves a cycle behind (scope -> increment -> scope).
// the heap has to stay bounded while the loop runs, not just get freed at exit
void testGarbageIsFreed() {
    Evaluator ev;
    run(ev, makeCounter + "for (let i = 0; i < 10000; i = i + 1) { makeCounter(); }");
    CHECK(liveEnvs(ev) < 2 * Heap::MIN_THRESHOLD);
    ev.collectGarbage();
    CHECK(liveEnvs(ev) == 1); // just globals
}

// a closure thats still referenced keeps its captured scope, and its state
void testLiveClosuresSurvive() {
    Evaluator ev;
    run(ev, makeCounter + "let c = makeCounter(); c(); c();");
    Environment* captured = closureOf(ev.environment->get("c", 0));
    ev.collectGarbage();
    CHECK(alive(ev, captured));
    run(ev, "let n = c();");
    CHECK(std::get<double>(ev.environment->get("n", 0)) == 3);
}

// marks have to get cleared after every collection, otherwise something that
// survived one gc could never be freed by a later one
void testMarksReset() {
    Evaluator ev;
    run(ev, makeCounter + "let c = makeCounter();");
    Environment* captured = closureOf(ev.environment->get("c", 0));
    ev.collectGarbage();
    CHECK(alive(ev, captured));
    run(ev, "c = null;");
    ev.collectGarbage();
    CHECK(liveEnvs(ev) == 1);
}

// 100k closures, each capturing the one before it. marking recursively would
// need c++ stack frames for every link, the gray stack doesnt
void testLongChain() {
    Evaluator ev;
    run(ev, R"(
fn link(prev) {
    fn node() {
        return prev;
    }
    return node;
}
let head = null;
for (let i = 0; i < 100000; i = i + 1) {
    head = link(head);
}
)");
    ev.collectGarbage();
    CHECK(liveEnvs(ev) > 100000);
    run(ev, "head = null;");
    ev.collectGarbage();
    CHECK(liveEnvs(ev) == 1);
}

// a runtime error deep in nested scopes has to unwind the scope stack, or the
// repl gets stuck in the inner scope and the dead scopes stay rooted forever
void testErrorUnwindsScopes() {
    Evaluator ev;
    Environment* globals = ev.environment;
    try {
        run(ev, "fn f() { for (let i = 0; i < 1; i = i + 1) { { print(nope); } } } f();");
    } catch (const GlyphError&) {}
    CHECK(ev.environment == globals);
    CHECK(ev.savedScopes.empty());
    CHECK(ev.tempRoots.empty());
    ev.collectGarbage();
    CHECK(liveEnvs(ev) == 1);
}

int main() {
    testGarbageIsFreed();
    testLiveClosuresSurvive();
    testMarksReset();
    testLongChain();
    testErrorUnwindsScopes();

    if (failures > 0) {
        std::cout << failures << " gc checks failed" << std::endl;
        return 1;
    }
    std::cout << "all gc checks passed" << std::endl;
    return 0;
}
