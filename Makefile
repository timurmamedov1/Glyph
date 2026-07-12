LANG = g++
FLAGS = -std=c++17 -Wall -Wextra -Werror -g
SRC = src/main.cpp
TARGET = glyph

$(TARGET): $(SRC)
	$(LANG) $(FLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	./run_tests.sh

clean:
	rm -f $(TARGET)

.PHONY: test clean
