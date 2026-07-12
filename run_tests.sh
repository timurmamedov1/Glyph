#!/bin/bash
# runs every test program and diffs its output against tests/expected/.
# exits nonzero on any failure so CI can catch regressions

cd "$(dirname "$0")"

if [ ! -x ./glyph ]; then
    echo "no glyph binary, run make first"
    exit 1
fi

pass=0
fail=0

for test in tests/*.gl; do
    name=$(basename "$test" .gl)
    expected="tests/expected/$name.out"

    if [ ! -f "$expected" ]; then
        echo "FAIL $name (no expected output file)"
        fail=$((fail + 1))
        continue
    fi

    # capture stderr too so error output regressions get caught
    actual=$(./glyph "$test" 2>&1)

    if [ "$actual" == "$(cat "$expected")" ]; then
        echo "PASS $name"
        pass=$((pass + 1))
    else
        echo "FAIL $name"
        diff <(cat "$expected") <(echo "$actual") | head -10
        fail=$((fail + 1))
    fi
done

echo ""
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
