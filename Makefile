LANG = g++
FLAGS = -std=c++17 -Wall -Wextra -Werror -g
# asan + ubsan, plus GC_STRESS so the gc runs on every allocation. a missing
# root turns into a use-after-free that asan catches right away
SANFLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -DGC_STRESS
SRC = src/main.cpp
# headers are listed so editing one triggers a rebuild, everything lives in them
HEADERS = $(wildcard src/*.h)
TARGET = glyph

$(TARGET): $(SRC) $(HEADERS)
	$(LANG) $(FLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./run_tests.sh

gc_test: tests/gc_test.cpp $(HEADERS)
	$(LANG) $(FLAGS) -o gc_test tests/gc_test.cpp

# checks the heap directly: garbage gets freed, live closures survive
gctest: gc_test
	./gc_test

glyph_san: $(SRC) $(HEADERS)
	$(LANG) $(FLAGS) $(SANFLAGS) -o glyph_san $(SRC)

# reruns every test program against the sanitizer build
sanitize: glyph_san
	GLYPH=./glyph_san ./run_tests.sh

clean:
	rm -f $(TARGET) gc_test glyph_san

.PHONY: test gctest sanitize clean
